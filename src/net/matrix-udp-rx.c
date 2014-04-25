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
#include "util.h"
#include "ledscape.h"


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


int
main(
	int argc,
	char ** argv
)
{
	const int port = 9999;
	const int sock = udp_socket(port);
	if (sock < 0)
		die("socket port %d failed: %s\n", port, strerror(errno));

	const unsigned width = 512;
	const unsigned height = 64;
	const size_t image_size = width * height * 3;

	// largest possible UDP packet
	uint8_t buf[width*height*4];
#if 0
	if (sizeof(buf) < image_size + 1)
		die("%u x %u too large for UDP\n", width, height);
#endif

	fprintf(stderr, "%u x %u, UDP port %u\n", width, height, port);

	ledscape_config_t * config = &ledscape_matrix_default;
	if (argc > 1)
	{
		config = ledscape_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
	}

	if (config->type == LEDSCAPE_MATRIX)
	{
		config->matrix_config.width = width;
		config->matrix_config.height = height;
	}

	ledscape_t * const leds = ledscape_init(config);
	if (!leds)
		return EXIT_FAILURE;

	const unsigned report_interval = 10;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;

	uint32_t * const fb = calloc(width*height,4);
	ledscape_draw(leds, fb);

	while (1)
	{
		const ssize_t rlen = recv(sock, buf, sizeof(buf), 0);
		if (rlen < 0)
			die("recv failed: %s\n", strerror(errno));
		warn_once("received %zu bytes\n", rlen);

		/*
		if (buf[0] == 2)
		{
			// image type
			printf("image type: %.*s\n",
				(int) rlen - 1,
				&buf[1]
			);
			continue;
		}

		if (buf[0] != 1)
		{
			// What is it?
			warn_once("Unknown image type '%c' (%02x)\n",
				buf[0],
				buf[0]
			);
			continue;
		}
		*/
		const unsigned frame_part = buf[0];
		if (frame_part != 0 && frame_part != 1)
		{
			printf("bad type %d\n", frame_part);
			continue;
		}

		if ((size_t) rlen != image_size + 1)
		{
			warn_once("WARNING: Received packet %zu bytes, expected %zu\n",
				rlen,
				image_size + 1
			);
		}

		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		const unsigned frame_num = 0;

		// copy the 3-byte values into the 4-byte framebuffer
		// and turn onto the side
		for (unsigned x = 0 ; x < width ; x++) // 256
		{
			for (unsigned y = 0 ; y < 32 ; y++) // 64
			{
				uint32_t * out = (void*) &fb[(y+32*frame_part)*width + x];
				const uint8_t * const in = &buf[1 + 3*(y*width + x)];
				uint32_t r = in[0];
				uint32_t g = in[1];
				uint32_t b = in[2];
				*out = (r << 16) | (g << 8) | (b << 0);
			}
		}

		// only draw after the second frame
		if (frame_part == 1)
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
