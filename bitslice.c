/** \file
 * Slice an RGB image into an 8-pixel GRB packed format for the Teensy.
 *
 * Designed to clock data out to USB teensy3 serial devices, which require
 * bitslicing into the raw form for the OctoWS2811 firmware.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include "bitslice.h"


/** Extract a ADDRESSING_HORIZONTAL_NORMAL pixel from the image.
 *
 * This reverses the y since the teensy's are at the bottom of the
 * strips, while the image coordinate frame is at the top.
 */
static const uint8_t *
bitmap_pixel(
	const uint8_t * const bitmap,
	const unsigned width,
	const unsigned height,
	const unsigned x,
	const unsigned y
)
{
	if (1)
		return bitmap + ((height - y - 1) * width + x) * 3;
	else
		return bitmap + ((height - y - 1) + x * height) * 3;
}


/** Prepare one Teensy3 worth of image data.
 *
 * Each Teensy handles 8 rows of data and needs the bits sliced into
 * each 8 rows.
 *
 * Since some of the pixels might have bad single channels,
 * allow them to be masked out entirely.
 */
void
bitslice(
	uint8_t * const out,
	const uint8_t * const bad_pixels,
	const uint8_t * in,
	const unsigned width,
	const unsigned height,
	const unsigned x_offset
)
{
	// Reorder from RGB in the input to GRB in the output
	static const uint8_t channel_map[] = { 1, 0, 2 };

	for(unsigned y=0 ; y < height ; y++)
	{
		for (unsigned channel = 0 ; channel < 3 ; channel++)
		{
			const uint8_t mapped_channel
				= channel_map[channel];

			for (unsigned bit_num = 0 ; bit_num < 8 ; bit_num++)
			{
				uint8_t b = 0;
				const uint8_t mask = 1 << (7 - bit_num);

				for(unsigned x = 0 ; x < 8 ; x++)
				{
					const uint8_t bit_pos = 1 << x;
					if (bad_pixels
					&&  bad_pixels[y] & bit_pos)
						continue;
					const uint8_t * const p
						= bitmap_pixel(in, width, height, x + x_offset, y);
					const uint8_t v = p[channel];

					// If the bit_num'th bit in v is
					// set, then mark that the x'th bit
					// in this output byte should be set.
					if (v & mask)
						b |= bit_pos;
				}

				out[24*y + 8*mapped_channel + bit_num] = b;
			}
		}
	}
}
