/** \file
 * Test the ledscape library by pulsing RGB on the first three LEDS.
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
	const int num_pixels = 128;
	ledscape_t * const leds = ledscape_init(num_pixels);
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	while (1)
	{
		// Alternate frame buffers on each draw command
		const unsigned frame_num = i++ % 2;
		ledscape_frame_t * const frame
			= ledscape_frame(leds, frame_num);

		uint8_t val = i >> 1;
		uint16_t r = ((i >>  0) & 0xFF);
		uint16_t g = ((i >>  8) & 0xFF);
		uint16_t b = ((i >> 16) & 0xFF);
		ledscape_fill_color(frame, num_pixels, val, val, val);

		for (unsigned strip = 0 ; strip < 32 ; strip++)
		{
			for (unsigned p = 0 ; p < 64 ; p++)
			{
				ledscape_set_color(
					frame,
					strip,
					p,
#if 1
					((strip % 3) == 0) ? (i) : 0,
					((strip % 3) == 1) ? (i) : 0,
					((strip % 3) == 2) ? (i) : 0
#else
					((strip % 3) == 0) ? 100 : 0,
					((strip % 3) == 1) ? 100 : 0,
					((strip % 3) == 2) ? 100 : 0
#endif
				);
				//ledscape_set_color(frame, strip, 3*p+1, 0, p+val + 80, 0);
				//ledscape_set_color(frame, strip, 3*p+2, 0, 0, p+val + 160);
			}
		}

		// wait for the previous frame to finish;
		const uint32_t response = ledscape_wait(leds);
		time_t now = time(NULL);
		if (now != last_time)
		{
			printf("%d fps. starting %d previous %"PRIx32"\n",
				i - last_i, i, response);
			last_i = i;
			last_time = now;
		}

		ledscape_draw(leds, frame_num);
	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
