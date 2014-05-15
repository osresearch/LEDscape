/** \file
 *  UDP image packet receiver.
 *
 * Based on the HackRockCity LED Display code:
 * https://github.com/agwn/pyramidTransmitter/blob/master/LEDDisplay.pde
 *
 * Designed to render into the LED matrix.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <getopt.h>
#include "util.h"
#include "ledscape.h"

static int verbose;

static int
udp_socket(
	const int port
)
{
	const int sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};

	if (sock < 0)
		return -1;
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		return -1;

	return sock;
}


static int
wait_socket(
	int fd,
	int msec_timeout
)
{
	struct timeval tv = { msec_timeout / 1000, msec_timeout % 1000 };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	return select(fd+1, &fds, NULL, NULL, &tv);
}


static struct option long_options[] =
{
	/* These options set a flag. */
	{"verbose", no_argument,       0, 1},
	{"port",    required_argument, 0, 'p'},
	{"width",   required_argument, 0, 'W'},
	{"height",  required_argument, 0, 'H'},
	{"config",  required_argument, 0, 'c'},
	{"timeout", required_argument, 0, 't'},
	{"message", required_argument, 0, 'm'},
	{"noinit",  no_argument,       0, 'n'},
	{0, 0, 0, 0}
};


static void usage(void)
{
	fprintf(stderr, "usage not yet written\n");
	exit(EXIT_FAILURE);
}


unsigned int packets_per_frame = 2;

int
main(
	int argc,
	char ** argv
)
{
	/* getopt_long stores the option index here. */
	int option_index = 0;
	int port = 9999;
	const char * config_file = NULL;
	const char * startup_message = "";
	int timeout = 60;
	unsigned width = 512;
	unsigned height = 64;
	int no_init = 0;

	while (1)
	{
		const int c = getopt_long(
			argc,
			argv,
			"vp:c:t:W:H:m:n",
			long_options,
			&option_index
		);

		if (c == -1)
			break;
		switch (c)
		{
		case 'v':
			verbose++;
			break;
		case 'n':
			no_init++;
			break;
		case 'c':
			config_file = optarg;
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case 'W':
			width = atoi(optarg);
			break;
		case 'H':
			height = atoi(optarg);
			break;
		case 'm':
			startup_message = optarg;
			break;
		default:
			usage();
			return -1;
		}
	}

	const int sock = udp_socket(port);
	if (sock < 0)
		die("socket port %d failed: %s\n", port, strerror(errno));

	const size_t image_size = width * height * 3;
	const size_t buf_size = (width*height*4)/packets_per_frame + 1;

	// largest possible UDP packet
	uint8_t *buf = malloc(buf_size);
#if 0
	if (sizeof(buf) < image_size + 1)
		die("%u x %u too large for UDP\n", width, height);
#endif

	fprintf(stderr, "%u x %u, UDP port %u\n", width, height, port);

	ledscape_config_t * config = &ledscape_matrix_default;
	if (config_file)
	{
		config = ledscape_config(config_file);
		if (!config)
			return EXIT_FAILURE;
	}

	if (config->type == LEDSCAPE_MATRIX)
	{
		config->matrix_config.width = width;
		config->matrix_config.height = height;
	}

	ledscape_t * const leds = ledscape_init(config, no_init);
	if (!leds)
		return EXIT_FAILURE;

	const unsigned report_interval = 10;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;

	uint32_t * const fb = calloc(width*height,4);
	ledscape_printf(fb, width, 0xFF0000, "%s", startup_message);
	ledscape_printf(fb+16*width, width, 0x00FF00, "%dx%d UDP port %d", width, height, port);
	ledscape_draw(leds, fb);

	while (1)
	{
		int rc = wait_socket(sock, timeout*1000);
		if (rc < 0)
		{
			// something failed
			memset(fb, 0, width*height*4);
			ledscape_printf(fb, width, 0xFF0000, "read failed?");
			ledscape_draw(leds, fb);
			exit(EXIT_FAILURE);
		}

		if (rc == 0)
		{
			// go into timeout mode
			memset(fb, 0, width*height*4);
			ledscape_printf(fb, width, 0xFF0000, "timeout");
			ledscape_draw(leds, fb);
			continue;
		}

		const ssize_t rlen = recv(sock, buf, buf_size, 0);
		if (rlen < 0)
			die("recv failed: %s\n", strerror(errno));
		warn_once("received %zu bytes\n", rlen);

		const unsigned frame_part = buf[0];
		if (frame_part == 0xff) {
			// Set packets_per_frame
			if ((size_t)rlen != 2) {
				warn("WARNING: Recieved PPF packet of %zu bytes, expected 2\n",rlen);
			} else {
				packets_per_frame = buf[1];
			}
			continue;
		} else if (frame_page >= packets_per_frame) {
			printf("frame part out of range: %d (max %d)\n", frame_part, packets_per_frame);
			continue;
		}

		if ((size_t) rlen != image_part_size + 1)
		{
			warn_once("WARNING: Received packet %zu bytes, expected %zu\n",
				rlen,
				image_part_size + 1
			);
		}

		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		const unsigned frame_num = 0;

		// copy the 3-byte values into the 4-byte framebuffer
		// and turn onto the side
		unsigned int pixels_per_packet = (width * height)/packets_per_frame;
		unsigned fb_offset = pixels_per_packet * frame_part;

		for (unsigned i = 0 ; i < pixels_per_packet ; i++) {
			const uint8_t * const in = buf + 1 + 3*i;
			uint32_t r = in[0];
			uint32_t g = in[1];
			uint32_t b = in[2];
			fb[fb_offset+i] = (r << 16) | (g << 8) | (b << 0);
		}

		// only draw after the second frame
		if (frame_part == packets_per_frame-1)
			ledscape_draw(leds, fb);

		gettimeofday(&stop_tv, NULL);
		timersub(&stop_tv, &start_tv, &delta_tv);

		frames++;
		delta_sum += delta_tv.tv_usec;
		if (stop_tv.tv_sec - last_report < report_interval)
			continue;
		last_report = stop_tv.tv_sec;

		const unsigned delta_avg = delta_sum / frames;
		printf("%6u usec avg, max %.2f fps, actual %.2f fps (over %u frames)\n",
			delta_avg,
			report_interval * 1.0e6 / delta_avg,
			frames * 1.0 / report_interval,
			frames
		);

		frames = delta_sum = 0;
	}

	return 0;
}
