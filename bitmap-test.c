/** \file
 * Test the matrix LCD PRU firmware by displaying an image.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"

int
main(
	int argc,
	char ** argv
)
{
	const int num_pixels = 256;
	ledscape_t * const leds = ledscape_init(num_pixels);

	FILE * f = fopen(argv[1], "r");
	uint8_t * buf = calloc(1<<20, 1);
	size_t len = fread(buf, 1, 1<<20, f);
	printf("len=%zu\n", len);

	buf += 0x82; // skip the BMP header (hardcoded)

	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	while (1)
	{
		// Alternate frame buffers on each draw command
		const unsigned frame_num = i++ % 2;
		ledscape_frame_t * const frame
			= ledscape_frame(leds, frame_num);

		uint32_t * const p = (void*) frame;

		for (unsigned x = 0 ; x < num_pixels ; x++)
		{
			for (unsigned y = 0 ; y < 16 ; y++)
			{
				uint8_t j = (i >> 6);
				uint8_t * const inpx = (void*) &buf[4 * ((x + j) % num_pixels) + num_pixels * (16 - y) * 4];
				uint8_t * const px = (void*) &p[x + num_pixels * y];

				px[0] = inpx[0];
				px[1] = inpx[1];
				px[2] = inpx[2];
			}
		}
		ledscape_draw(leds, frame_num);
		usleep(100);

		// wait for the previous frame to finish;
		//const uint32_t response = ledscape_wait(leds);
		const uint32_t response = 0;
		time_t now = time(NULL);
		if (now != last_time)
		{
			printf("%d fps. starting %d previous %"PRIx32"\n",
				i - last_i, i, response);
			last_i = i;
			last_time = now;
		}

	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
