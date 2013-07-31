/** \file
 * LEDscape for the BeagleBone Black.
 *
 * Drives up to 32 ws281x LED strips using the PRU to have no CPU overhead.
 * Allows easy double buffering of frames.
 */

#ifndef _ledscape_h_
#define _ledscape_h_

#include <stdint.h>

/** The number of strips supported.
 *
 * Changing this also requires changes in ws281x.p to stride the
 * correct number of bytes per row..
 */
#define LEDSCAPE_NUM_STRIPS 32


/** LEDscape pixel format is BRGA.
 *
 * data is laid out with BRGA format, since that is how it will
 * be translated during the clock out from the PRU.
 */
typedef struct {
	uint8_t b;
	uint8_t r;
	uint8_t g;
	uint8_t a;
} __attribute__((__packed__)) ledscape_pixel_t;


/** LEDscape frame buffer is "strip-major".
 *
 * All 32 strips worth of data for each pixel are stored adjacent.
 * This makes it easier to clock out while reading from the DDR
 * in a burst mode.
 */
typedef struct {
	ledscape_pixel_t strip[LEDSCAPE_NUM_STRIPS];
} __attribute__((__packed__)) ledscape_frame_t;


typedef struct ledscape ledscape_t;


extern ledscape_t *
ledscape_init(
	unsigned num_pixels
);


extern ledscape_frame_t *
ledscape_frame(
	ledscape_t * const leds,
	unsigned frame
);


extern void
ledscape_draw(
	ledscape_t * const leds,
	unsigned frame
);


extern void
ledscape_set_color(
	ledscape_frame_t * const frame,
	uint8_t strip,
	uint8_t pixel,
	uint8_t r,
	uint8_t g,
	uint8_t b
);


extern uint32_t
ledscape_wait(
	ledscape_t * const leds
);


extern void
ledscape_close(
	ledscape_t * const leds
);

#endif
