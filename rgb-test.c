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

static void
ledscape_fill_color(
	ledscape_frame_t * const frame,
	const unsigned num_pixels,
	const uint8_t r,
	const uint8_t g,
	const uint8_t b
)
{
	for (unsigned i = 0 ; i < num_pixels ; i++)
		for (unsigned strip = 0 ; strip < LEDSCAPE_NUM_STRIPS ; strip++)
			ledscape_set_color(frame, strip, i, r, g, b);
}


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
		ledscape_fill_color(frame, num_pixels, val, 0, val);

		for (int strip = 0 ; strip < 32 ; strip++)
		{
			ledscape_set_color(frame, strip, 0, val, 0, 0);
			ledscape_set_color(frame, strip, 1, 0, val + 80, 0);
			ledscape_set_color(frame, strip, 2, 0, 0, val + 160);
		}

		// wait for the previous frame to finish;
		const uint32_t response = ledscape_wait(leds);
		printf("starting %d previous %"PRIx32"\n", i, response);

		ledscape_draw(leds, frame_num);
	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
