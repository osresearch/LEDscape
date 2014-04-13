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
#include "bitslice.h"
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

	const unsigned width = 256;
	const unsigned height = 64;
	const unsigned leds_width = 256;
	const unsigned leds_height = 128;
	const size_t image_size = width * height * 3;

	// largest possible UDP packet
	uint8_t buf[65536];
	if (sizeof(buf) < image_size + 1)
		die("%u x %u too large for UDP\n", width, height);

	fprintf(stderr, "%u x %u, UDP port %u\n", width, height, port);

	ledscape_t * const leds = ledscape_init(leds_width, leds_height);

	const unsigned report_interval = 10;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;

	uint32_t * const fb = calloc(width*height,4);
	uint32_t * const leds_fb = calloc(leds_width*leds_height,4);
	ledscape_draw(leds, leds_fb);

	while (1)
	{
		const ssize_t rlen = recv(sock, buf, sizeof(buf), 0);
		if (rlen < 0)
			die("recv failed: %s\n", strerror(errno));
		warn_once("received %zu bytes\n", rlen);

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
			for (unsigned y = 0 ; y < height ; y++) // 64
			{
				uint8_t * out = (void*) &fb[y*2*leds_width + x];
				const uint8_t * const in = &buf[1 + 3*(x*height + (width - y - 1))];
				out[0] = in[0];
				out[1] = in[1];// / 2;
				out[2] = in[2];// / 2;

				if (0) {
					// double the pixels?
					out += leds_width * 4;
					out[0] = in[0];
					out[1] = in[1] / 2;
					out[2] = in[2] / 2;
				}
			}
		}

#if 0
		// the panel is 256x128, the pyramid was 256x64.
		framebuffer_flip(leds_fb, fb + width*0, leds_width, leds_height, width, height-16);
		ledscape_draw(leds, leds_fb);
#else
		ledscape_draw(leds, fb);
#endif

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
