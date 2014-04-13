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


typedef struct {
	int x;
	int y;
	int rot; // 0 == none, 1 == left, 2 == right, 3 == flip
} ledscape_matrix_panel_t;

#define LEDSCAPE_MATRIX_OUTPUTS 8 // number of outputs on the cape
#define LEDSCAPE_MATRIX_PANELS 8 // number of panels chained per output

typedef struct {
	int width;
	int height;
	int panel_width;
	int panel_height;
	int leds_width;
	int leds_height;
	ledscape_matrix_panel_t panels[LEDSCAPE_MATRIX_OUTPUTS][LEDSCAPE_MATRIX_PANELS];
} ledscape_matrix_config_t;


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
	unsigned width,
	unsigned height
);


extern void
ledscape_draw(
	ledscape_t * const leds,
	const void * const rgb // 4-byte rgb data
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


/** Flip a rectangular frame buffer to map the LED matrices */
void
ledscape_matrix_remap(
	uint32_t * leds_out,
	const uint32_t * fb_in,
	const ledscape_matrix_config_t * config
);


/** Write with a fixed-width 8px font */
void
ledscape_printf(
	uint32_t * px,
	const size_t width,
	const uint32_t color,
	const char * fmt,
	...
);

/** Parse a matrix config file */
ledscape_matrix_config_t *
ledscape_matrix_config(
	const char * filename
);

#endif
