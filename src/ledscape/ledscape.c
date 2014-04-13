/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"
#include "pru.h"

#define CONFIG_LED_MATRIX


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
	23, 27, 22, 10, 9, 8, 26, 11, 30, 31, 5, 3, 20, 4, 2, 14, 7, 15
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


/*
 * Configure all of our output pins.
 * These must have also been set by the device tree overlay.
 * If they are not, some things will appear to work, but not
 * all the output pins will be correctly configured as outputs.
 */
static void
ledscape_gpio_init(void)
{
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios0) ; i++)
		pru_gpio(0, gpios0[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios1) ; i++)
		pru_gpio(1, gpios1[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios2) ; i++)
		pru_gpio(2, gpios2[i], 1, 0);
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios3) ; i++)
		pru_gpio(3, gpios3[i], 1, 0);
}


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


static uint8_t *
ledscape_remap(
	ledscape_t * const leds,
	uint8_t * const frame,
	unsigned x,
	unsigned y
)
{
#define CONFIG_ZIGZAG
#ifdef CONFIG_ZIGZAG
	(void) leds;

	// each panel is 16x8
	// vertical panel number is y % 8 (which output line)
	// horizontal panel number is y % (16*8)
	// if y % 2 == 1, map backwards
	const unsigned panel_width = 16;
	const unsigned panel_height = 8;
	unsigned panel_num = x / panel_width;
	unsigned output_line = y / panel_height;
	unsigned panel_x = x % panel_width;
	unsigned panel_y = y % panel_height;
	unsigned panel_offset = panel_y * panel_width;

	// the even lines are forwards, the odd lines go backwards
	if (panel_y % 2 == 0)
	{
		panel_offset += panel_x;
	} else {
		panel_offset += panel_width - panel_x - 1;
	}

	return &frame[(panel_num*128 + panel_offset)*48*3 + output_line];
#else
	return &frame[x*48*3 + y];
#endif
}


static void
ledscape_matrix_draw(
	ledscape_t * const leds,
	const void * const buffer
)
{
	static unsigned frame = 0;
	const uint32_t * const in = buffer;
	uint8_t * const out = leds->pru->ddr + leds->frame_size * frame;

	// matrix packed is:
	// this way the PRU can read all sixteen output pixels in
	// one LBBO and clock them out.
	// there is an array of NUM_MATRIX output coordinates (one for each of
	// the sixteen drivers).
	for (unsigned i = 0 ; i < NUM_MATRIX ; i++)
	{
		const led_matrix_t * const m = &leds->matrix->matrix[i];

		for (uint32_t y = 0 ; y < leds->matrix->matrix_height ; y++)
		{
			const uint32_t * const in_row
				= &in[(y+m->y_offset) * leds->width];
			uint8_t * const out_row
				= &out[y * leds->matrix->matrix_width * 3 * NUM_MATRIX];

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
	leds->ws281x->pixels_dma = leds->pru->ddr_addr + leds->frame_size * frame;
	// disable double buffering for now
	//frame = (frame + 1) & 1;
}


/** Translate the RGBA buffer to the correct output type and
 * initiate the transfer of a frame to the LED strips.
 *
 * Matrix drivers shuffle to have consecutive bits, ws281x do bit slicing.
 */
void
ledscape_strip_draw(
	ledscape_t * const leds,
	const void * const buffer
)
{
	static unsigned frame = 0;
	const uint32_t * const in = buffer;
	uint8_t * const out = leds->pru->ddr + leds->frame_size * frame;

	// Translate the RGBA frame into G R B, sliced by color
	// only 48 outputs currently supported
	const unsigned pru_stride = 48;
	for (unsigned y = 0 ; y < leds->height ; y++)
	{
		const uint32_t * const row_in = &in[y*leds->width];
		for (unsigned x = 0 ; x < leds->width ; x++)
		{
			uint8_t * const row_out
				= ledscape_remap(leds, out, x, y);
			const uint32_t p = row_in[x];
			row_out[0*pru_stride] = (p >>  8) & 0xFF; // green
			row_out[1*pru_stride] = (p >>  0) & 0xFF; // red
			row_out[2*pru_stride] = (p >> 16) & 0xFF; // blue
		}
	}
	
	// Wait for any current command to have been acknowledged
	while (leds->ws281x->command)
		;

	// Update the pixel data and send the start
	leds->ws281x->pixels_dma
		= leds->pru->ddr_addr + leds->frame_size * frame;
	frame = (frame + 1) & 1;

	// Send the start command
	leds->ws281x->command = 1;
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

static ledscape_t *
ledscape_matrix_init(
	unsigned width,
	unsigned height
)
{
	pru_t * const pru = pru_init(0);
	const size_t frame_size = 16 * 8 * width * 3; //LEDSCAPE_NUM_STRIPS * 4;

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
		.matrix_width	= 256,
		.matrix_height	= 8,
		.matrix		= {
			{ 0, 0 },
			{ 0, 8 },

			{ 0, 16 },
			{ 0, 24 },

			{ 0, 32 },
			{ 0, 40 },

			{ 0, 48 },
			{ 0, 56 },

			{ 0, 64 },
			{ 0, 72 },

			{ 0, 80 },
			{ 0, 88 },

			{ 0, 96 },
			{ 0, 104 },

			{ 0, 112 },
			{ 0, 120 },
		},
	};

	*(leds->ws281x) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.num_pixels	= (leds->matrix->matrix_width * 3) * 16,
		.command	= 0,
		.response	= 0,
	};

	ledscape_gpio_init();

	// Initiate the PRU program
	pru_exec(pru, "./lib/matrix.bin");

	// Watch for a done response that indicates a proper startup
	// \todo timeout if it fails
	printf("waiting for response\n");
	while (!leds->ws281x->response)
		;
	printf("got response\n");

	return leds;
}


static ledscape_t *
ledscape_strips_init(
	unsigned width,
	unsigned height
)
{
	pru_t * const pru = pru_init(0);
	const size_t frame_size = 48 * width * 8 * 3;

	printf("frame-size %zu, ddr-size=%zu\n", frame_size, pru->ddr_size);
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

	// LED strips, not matrix output
	*(leds->ws281x) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.num_pixels	= width * 8, // panel height
		.command	= 0,
		.response	= 0,
	};

	printf("%d\n", leds->ws281x->num_pixels);

	ledscape_gpio_init();

	// Initiate the PRU program
	pru_exec(pru, "./lib/ws281x.bin");

	// Watch for a done response that indicates a proper startup
	// \todo timeout if it fails
	printf("waiting for response\n");
	while (!leds->ws281x->response)
		;
	printf("got response\n");

	return leds;
}


ledscape_t *
ledscape_init(
	unsigned width,
	unsigned height
)
{
	return ledscape_matrix_init(width, height);
}


void
ledscape_draw(
	ledscape_t * const leds,
	const void * const buffer
)
{
	return ledscape_matrix_draw(leds, buffer);
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



/** Copy a 16x32 region from in to a 32x16 region of out.
 * If rot == 0, rotate -90, else rotate +90.
 */
static void
ledscape_matrix_panel_copy(
	uint32_t * const out,
	const uint32_t * const in,
	const ledscape_matrix_config_t * const config,
	const int rot
)
{
	for (int x = 0 ; x < config->panel_width ; x++)
	{
		for (int y = 0 ; y < config->panel_height ; y++)
		{
			int ox, oy;
			if (rot == 0)
			{
				// no rotation == (0,0) => (0,0)
				ox = x;
				oy = y;
			} else
			if (rot == 1)
			{
				// rotate +90 (0,0) => (0,15)
				ox = config->panel_height-1 - y;
				oy = x;
			} else
			if (rot == 2)
			{
				// rotate -90 (0,0) => (31,0)
				ox = y;
				oy = config->panel_width-1 - x;
			} else
			if (rot == 3)
			{
				// flip == (0,0) => (31,15)
				ox = config->panel_width-1 - x;
				oy = config->panel_height-1 - y;
			} else
			{
				// barf
				ox = oy = 0;
			}

			out[x + config->leds_width*y]
				= in[ox + config->width*oy];
		}
	}
}


/** Remap the matrix framebuffer based on the config.
 */
void
ledscape_matrix_remap(
	uint32_t * const out,
	const uint32_t * const in,
	const ledscape_matrix_config_t * const config
)
{
	for (int i = 0 ; i < LEDSCAPE_MATRIX_OUTPUTS ; i++)
	{
		for (int j = 0 ; j < LEDSCAPE_MATRIX_PANELS ; j++)
		{
			const ledscape_matrix_panel_t * const panel
				= &config->panels[i][j];

			int ox = config->panel_width * j;
			int oy = config->panel_height * i;
			//printf("%d,%d => %d,%d <= %d,%d,%d\n", i, j, ox, oy, panel->x, panel->y, panel->rot);

			const uint32_t * const ip
				= &in[panel->x + panel->y * config->width];
			uint32_t * const op
				= &out[ox + oy * config->leds_width];
		
			ledscape_matrix_panel_copy(
				op,
				ip,
				config,
				panel->rot
			);
		}
	}
}


extern const uint8_t fixed_font[][5];

void
ledscape_draw_char(
	uint32_t * px,
	const size_t width,
	const uint32_t color,
	char c
)
{
	if (c < 0x20 || c > 127)
		c = '?';

	const uint8_t* const f = fixed_font[c - 0x20];
	for (int i = 0 ; i < 5 ; i++, px++)
	{
		uint8_t bits = f[i];
		for (int j = 0 ; j < 7 ; j++, bits >>= 1)
			px[j*width] = bits & 1 ? color : 0;
	}
}


/** Write with a fixed-width 8px font */
void
ledscape_printf(
	uint32_t * px,
	const size_t width,
	const uint32_t color,
	const char * fmt,
	...
)
{
	char buf[128];
	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	printf("%p => '%s'\n", px, buf);
	for (unsigned i = 0 ; i < sizeof(buf) ; i++)
	{
		char c = buf[i];
		if (!c)
			break;
		ledscape_draw_char(px, width, color, c);
		px += 6;
	}
}

