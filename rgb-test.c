/** \file
 * Test the ledscape library by pulsing RGB on the first three LEDS.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"


int main (void)
{
	const int num_pixels = 256;
	ledscape_t * const leds = ledscape_init(num_pixels);

	unsigned i = 0;
	while (1)
	{
		// Alternate frame buffers on each draw command
		const unsigned frame_num = i++ % 2;
		ledscape_frame_t * const frame
			= ledscape_frame(leds, frame_num);

		uint8_t val = i >> 1;
		fill_color(frame, num_pixels, val, 0, val);

		for (int strip = 0 ; strip < 32 ; strip++)
		{
			//uint8_t r = ((strip >> 2) & 0x3) * 64;
			//uint8_t g = ((strip >> 0) & 0x3) * 64;
			//uint8_t b = ((strip >> 4) & 0x3) * 64;
			ledscale_set_color(frame, strip, 0, val, 0, 0);
			ledscale_set_color(frame, strip, 1, 0, val + 80, 0);
			ledscale_set_color(frame, strip, 2, 0, 0, val + 160);
		}

		// wait for the previous frame to finish;
		const uint32_t response = ledscape_wait(leds);
		printf("starting %d previous %"PRIx32"\n", i, response);

		ledscape_draw(leds, frame_num);
	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
