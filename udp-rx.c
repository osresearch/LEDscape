/** \file
 *  UDP image packet receiver.
 *
 * Based on the HackRockCity LED Display code:
 * https://github.com/agwn/pyramidTransmitter/blob/master/LEDDisplay.pde
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include "ledscape.h"
#include "pru.h"

int
main(
	int argc,
	char ** argv
)
{
	int port = 9999;
	int num_pixels = 256;
	int num_strips = 32; // not necessarily LEDSCAPE_NUM_STRIPS

	const int sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(port),
	};

	if (sock < 0)
		die("socket failed: %s\n", strerror(errno));
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		die("bind port %d failed: %s\n", port, strerror(errno));

	ledscape_t * const leds = ledscape_init(num_pixels);

	unsigned frame_num = 0;

	uint8_t buf[65536];

	while (1)
	{
		const ssize_t rc = recv(sock, buf, sizeof(buf), 0);
		if (rc < 0)
			die("recv failed: %s\n", strerror(errno));

		if (buf[0] == '2')
		{
			// image type
			printf("image type: %.*s\n",
				(int) rc - 1,
				&buf[1]
			);
			continue;
		}

		ledscape_frame_t * const frame
			= ledscape_frame(leds, frame_num);

		for(unsigned x=0 ; x < num_pixels ; x++)
		{
			for(unsigned strip = 0 ; strip < 32 ; strip++)
			{
				const uint8_t r = buf[x*num_strips+1];
				const uint8_t g = buf[x*num_strips+2];
				const uint8_t b = buf[x*num_strips+3];
				ledscape_set_color(frame, strip, x, r, g, b);
			}
		}

		ledscape_wait(leds);
		ledscape_draw(leds, frame_num);

		frame_num = (frame_num+1) % 2;
	}

	ledscape_close(leds);
	return 0;
}
