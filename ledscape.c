/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"
#include "pru.h"


/** GPIO pins used by the LEDscape.
 *
 * The device tree should handle this configuration for us, but it
 * seems horribly broken and won't configure these pins as outputs.
 * So instead we have to repeat them here as well.
 *
 * If these are changed, be sure to check the mappings in
 * ws281x.p!
 *
 * The RGB matrix uses a subset of these pins, although with
 * the HDMI disabled it might use quite a few more for the four
 * output version.
 *
 * \todo: Find a way to unify this with the defines in the .p file
 */
static const uint8_t gpios0[] = {
	23, 27, 22, 10, 9, 8, 26, 11, 30, 31, 5, 3, 20, 4, 2, 14, 7
};

static const uint8_t gpios1[] = {
	13, 15, 12, 14, 29, 16, 17, 28, 18, 19,
};

static const uint8_t gpios2[] = {
	2, 5, 22, 23, 14, 12, 10, 8, 6, 3, 4, 1, 24, 25, 17, 16, 15, 13, 11, 9, 7,
};

static const uint8_t gpios3[] = {
	21, 19, 15, 14, 17, 16
};

#define ARRAY_COUNT(a) ((sizeof(a) / sizeof(*a)))


/** Command structure shared with the PRU.
 *
 * This is mapped into the PRU data RAM and points to the
 * frame buffer in the shared DDR segment.
 *
 * Changing this requires changes in ws281x.p
 */
typedef struct
{
	// in the DDR shared with the PRU
	uintptr_t pixels_dma;

	// Length in pixels of the longest LED strip.
	unsigned num_pixels;

	// write 1 to start, 0xFF to abort. will be cleared when started
	volatile unsigned command;

	// will have a non-zero response written when done
	volatile unsigned response;
} __attribute__((__packed__)) ws281x_command_t;

typedef struct
{
	uint32_t x_offset;
	uint32_t y_offset;
} led_matrix_t;

#define NUM_MATRIX 16

typedef struct
{
	uint32_t matrix_width; // of a full chain
	uint32_t matrix_height; // number of rows per-output (8 or 16)
	led_matrix_t matrix[NUM_MATRIX];
} led_matrix_config_t;



struct ledscape
{
	ws281x_command_t * ws281x;
	pru_t * pru;
	unsigned width;
	unsigned height;
	unsigned frame_size;
	led_matrix_config_t * matrix;
};


#if 0
/** Retrieve one of the two frame buffers. */
ledscape_frame_t *
ledscape_frame(
	ledscape_t * const leds,
	unsigned int frame
)
{
	if (frame >= 2)
		return NULL;

	return (ledscape_frame_t*)((uint8_t*) leds->pru->ddr + leds->frame_size * frame);
}
#endif


static uint8_t
bright_map(
	uint8_t val
)
{
	return val;
}


/** Translate the RGBA buffer to the correct output type and
 * initiate the transfer of a frame to the LED strips.
 *
 * Matrix drivers shuffle to have consecutive bits, ws281x do bit slicing.
 */
void
ledscape_draw(
	ledscape_t * const leds,
	const void * const buffer
)
{
	static unsigned frame = 0;
	const uint32_t * const in = buffer;
	uint8_t * const out = leds->pru->ddr + leds->frame_size * frame;

#if 1
	// matrix packed is:
	// p(0,0)p(0,8)p(64,0)p(64,8)....
	// this way the PRU can read all sixteen output pixels in
	// one LBBO and clock them out.
	// there is an array of NUM_MATRIX output coordinates (one for each of
	// the sixteen drivers).
	for (unsigned i = 0 ; i < NUM_MATRIX ; i++)
	{
		const led_matrix_t * const m = &leds->matrix->matrix[i];

		for (uint32_t y = 0 ; y < leds->matrix->matrix_height ; y++)
		{
			const uint32_t * const in_row = &in[(y+m->y_offset) * leds->width];
			uint8_t * const out_row = &out[y * leds->matrix->matrix_width * 3 * NUM_MATRIX];

			for (uint32_t x = 0 ; x < leds->matrix->matrix_width ; x++)
			{
				const uint8_t * const rgb = (const void*) &in_row[x + m->x_offset];
				uint8_t * const out_rgb = &out_row[(i + x * NUM_MATRIX)*3];
				out_rgb[0] = bright_map(rgb[0]);
				out_rgb[1] = bright_map(rgb[1]);
				out_rgb[2] = bright_map(rgb[2]);
			}
		}
	}
#else
	static int i = 0;
	memset(out, i++, leds->width *leds->height * 3);
	out[0] = 0x80; out[1] = 0x00; out[2] = 0x00;
	out[3] = 0x00; out[4] = 0x80; out[5] = 0x00;
	out[6] = 0x00; out[7] = 0x00; out[8] = 0x80;
#endif

	leds->ws281x->pixels_dma = leds->pru->ddr_addr + leds->frame_size * frame;
	frame = (frame + 1) & 1;
#if 0
	// Wait for any current command to have been acknowledged
	while (leds->ws281x->command)
		;

	// Send the start command
	leds->ws281x->command = 1;
#endif
}


/** Wait for the current frame to finish transfering to the strips.
 * \returns a token indicating the response code.
 */
uint32_t
ledscape_wait(
	ledscape_t * const leds
)
{
	while (1)
	{
		uint32_t response = leds->ws281x->response;
		if (!response)
			continue;
		leds->ws281x->response = 0;
		return response;
	}
}


ledscape_t *
ledscape_init(
	unsigned width,
	unsigned height
)
{
	pru_t * const pru = pru_init(0);
	const size_t frame_size = 16 * 8 * width * 3; //LEDSCAPE_NUM_STRIPS * 4;

#if 0
	if (2 *frame_size > pru->ddr_size)
		die("Pixel data needs at least 2 * %zu, only %zu in DDR\n",
			frame_size,
			pru->ddr_size
		);
#endif

	ledscape_t * const leds = calloc(1, sizeof(*leds));

	*leds = (ledscape_t) {
		.pru		= pru,
		.width		= width,
		.height		= height,
		.ws281x		= pru->data_ram,
		.frame_size	= frame_size,
		.matrix		= calloc(sizeof(*leds->matrix), 1),
	};

	*(leds->matrix) = (led_matrix_config_t) {
		.matrix_width	= 128,
		.matrix_height	= 8,
		.matrix		= {
#if 1
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 16 },
			{ 0, 24 },
			{ 0, 32 },
			{ 0, 40 },
			{ 0, 48 },
			{ 0, 56 },
			{ 128, 0 },
			{ 128, 8 },
			{ 128, 16 },
			{ 128, 24 },
			{ 128, 32 },
			{ 128, 40 },
			{ 128, 48 },
			{ 128, 56 },
#else
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
			{ 0, 0 },
			{ 0, 8 },
#endif
		},
	};

	*(leds->ws281x) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.num_pixels	= (leds->matrix->matrix_width * 3) * 16,
		.command	= 0,
		.response	= 0,
	};

	printf("%d\n", leds->ws281x->num_pixels);


	// Configure all of our output pins.
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios0) ; i++)
		pru_gpio(0, gpios0[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios1) ; i++)
		pru_gpio(1, gpios1[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios2) ; i++)
		pru_gpio(2, gpios2[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios3) ; i++)
		pru_gpio(3, gpios3[i], 1, 0);

	// Initiate the PRU program
	if (1)
		pru_exec(pru, "./matrix.bin");
	else
		pru_exec(pru, "./ws281x.bin");

	// Watch for a done response that indicates a proper startup
	// \todo timeout if it fails
	printf("waiting for response\n");
	while (!leds->ws281x->response)
		;
	printf("got response\n");

	return leds;
}


void
ledscape_close(
	ledscape_t * const leds
)
{
	// Signal a halt command
	leds->ws281x->command = 0xFF;
	pru_close(leds->pru);
}


void
ledscape_set_color(
	ledscape_frame_t * const frame,
	uint8_t strip,
	uint8_t pixel,
	uint8_t r,
	uint8_t g,
	uint8_t b
)
{
	ledscape_pixel_t * const p = &frame[pixel].strip[strip];
	p->r = r;
	p->g = g;
	p->b = b;
}
