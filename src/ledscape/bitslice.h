#ifndef _ledscape_bitslice_h_
#define _ledscape_bitslice_h_

#include <stdint.h>


/** Prepare one Teensy3 worth of image data.
 *
 * Each Teensy handles 8 rows of data and needs the bits sliced into
 * each 8 rows.
 *
 * Since some of the pixels might have bad single channels,
 * allow them to be masked out entirely.
 */
extern void
bitslice(
	uint8_t * const out,
	const uint8_t * const bad_pixels,
	const uint8_t * in,
	const unsigned width,
	const unsigned height,
	const unsigned x_offset
);

#endif
