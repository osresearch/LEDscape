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
static const uint8_t gpios_output1[] = {
	2,2,2,3,2,5,0,23,2,4,0,26
};

static const uint8_t gpios_output2[] = {
	0,27,2,1,0,22,2,22,2,23,2,24
};

static const uint8_t gpios_output3[] = {
	0,30,1,18,0,31,1,16,0,3,0,5
};

static const uint8_t gpios_output4[] = {
	0,2,0,15,1,17,3,21,3,19,0,4,1,19
};

static const uint8_t gpios_output5[] = {
	2,25,0,11,0,10,0,9,0,8,2,17
};

static const uint8_t gpios_output6[] = {
	2,16,2,15,2,14,2,13,2,10,2,12
};

static const uint8_t gpios_output7[] = {
	2,11,2,9,2,8,2,6,0,7,2,7
};

static const uint8_t gpios_output8[] = {
	3,17,3,16,3,15,3,14,0,14,0,20
};

static const uint8_t gpios_output_all[] = {
	1,12,1,13,1,14,1,15,1,28,1,29,1,19
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
	for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output1) ; i+=2)
		pru_gpio(gpios_output1[i], gpios_output1[i+1], 1, 0);
#if LEDSCAPE_MATRIX_OUTPUTS > 1
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output2) ; i+=2)
                pru_gpio(gpios_output2[i], gpios_output2[i+1], 1, 0);
#endif 
#if LEDSCAPE_MATRIX_OUTPUTS > 2
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output3) ; i+=2)
                pru_gpio(gpios_output3[i], gpios_output3[i+1], 1, 0);
#endif 
#if LEDSCAPE_MATRIX_OUTPUTS > 3
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output4) ; i+=2)
                pru_gpio(gpios_output4[i], gpios_output4[i+1], 1, 0);
#endif 
#if LEDSCAPE_MATRIX_OUTPUTS > 4
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output5) ; i+=2)
                pru_gpio(gpios_output5[i], gpios_output5[i+1], 1, 0);
#endif 
#if LEDSCAPE_MATRIX_OUTPUTS > 5
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output6) ; i+=2)
                pru_gpio(gpios_output6[i], gpios_output6[i+1], 1, 0);
#endif 
#if LEDSCAPE_MATRIX_OUTPUTS > 6
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output7) ; i+=2)
                pru_gpio(gpios_output7[i], gpios_output7[i+1], 1, 0);
#endif 
#if LEDSCAPE_MATRIX_OUTPUTS > 7
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output8) ; i+=2)
                pru_gpio(gpios_output8[i], gpios_output8[i+1], 1, 0);
#endif
        for (unsigned i = 0 ; i < ARRAY_COUNT(gpios_output_all) ; i+=2)
                pru_gpio(gpios_output_all[i], gpios_output_all[i+1], 1, 0);
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

#if 0
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
#endif



struct ledscape
{
	ws281x_command_t * ws281x;
	pru_t * pru;
	unsigned width;
	unsigned height;
	unsigned frame_size;
	ledscape_config_t * config;
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
#undef CONFIG_ZIGZAG
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
	(void) leds;
	return &frame[x*48*3 + y];
#endif
}


/** Copy a 16x32 region from in to a 32x16 region of out.
 * If rot == 0, rotate -90, else rotate +90.
 */
static void
ledscape_matrix_panel_copy(
	uint8_t * const out,
	const uint32_t * const in,
	const ledscape_matrix_config_t * const config,
	const int rot
)
{
	const size_t row_stride = LEDSCAPE_MATRIX_OUTPUTS*3*2;
	const size_t row_len = config->leds_width*row_stride;

	for (int x = 0 ; x < config->panel_width ; x++)
	{
		for (int y = 0 ; y < config->panel_height ; y++)
		{
			int ix, iy;
			if (rot == 0)
			{
				// no rotation == (0,0) => (0,0)
				ix = x;
				iy = y;
			} else
			if (rot == 1)
			{
				// rotate +90 (0,0) => (0,15)
				ix = config->panel_height-1 - y;
				iy = x;
			} else
			if (rot == 2)
			{
				// rotate -90 (0,0) => (31,0)
				ix = y;
				iy = config->panel_width-1 - x;
			} else
			if (rot == 3)
			{
				// flip == (0,0) => (31,15)
				ix = config->panel_width-1 - x;
				iy = config->panel_height-1 - y;
			} else
			{
				// barf
				ix = iy = 0;
			}

			const uint32_t * const col_ptr = &in[ix + config->width*iy];
			const uint32_t col = *col_ptr;

			// the top half and bottom half of the panels
			// are squished together in the output since
			// they are drawn simultaneously.
			uint8_t * const pix = &out[x*row_stride + (y/(config->panel_height/2))*3 + (y%(config->panel_height/2))*row_len];

			pix[0] = (col >> 16) & 0xFF; // red
			pix[1] = (col >>  8) & 0xFF; // green
			pix[2] = (col >>  0) & 0xFF; // blue
			//printf("%d,%d => %p %p %08x\n", x, y, pix, col_ptr, col);
		}
	}
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

	// matrix is re-packed such that a 6-byte read will bring in
	// the brightness values for all six outputs of a given panel.
	// this means that the rows stride 16 * 3 pixels at a time.
	// 
	// this way the PRU can read all sixteen output pixels in
	// one LBBO and clock them out.
	// while there are eight output chains, there are two simultaneous
	// per output chain.
	const ledscape_matrix_config_t * const config
		= &leds->config->matrix_config;

	const size_t panel_stride = config->panel_width*2*3*LEDSCAPE_MATRIX_OUTPUTS;

	for (unsigned i = 0 ; i < LEDSCAPE_MATRIX_OUTPUTS ; i++)
	{
		for (unsigned j = 0 ; j < LEDSCAPE_MATRIX_PANELS ; j++)
		{
			const ledscape_matrix_panel_t * const panel
				= &config->panels[i][j];

			// the start of the panel in the input
			// is defined by the panel's xy coordinate
			// and the width of the input image.
			const uint32_t * const ip
				= &in[panel->x + panel->y*config->width];

			// the start of the panel's output is defined
			// by the current output panel number and the total
			// number of panels in the chain.
			uint8_t * const op = &out[6*i + j*panel_stride];
		
			// copy the top half of this matrix
			ledscape_matrix_panel_copy(
				op,
				ip,
				config,
				panel->rot
			);
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
			uint32_t p = row_in[x];
			row_out[0*pru_stride] = (p >>  8) & 0xFF; // green
			row_out[1*pru_stride] = (p >> 16) & 0xFF; // red
			row_out[2*pru_stride] = (p >>  0) & 0xFF; // blue
		}
	}
	
	// Wait for any current command to have been acknowledged
	while (leds->ws281x->command)
		;

	// Update the pixel data and send the start
	leds->ws281x->pixels_dma
		= leds->pru->ddr_addr + leds->frame_size * frame;
//	frame = (frame + 1) & 1;

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
	ledscape_config_t * const config_union,
	int no_pru_init
)
{
	ledscape_matrix_config_t * const config = &config_union->matrix_config;
	pru_t * const pru = pru_init(0);
	const size_t frame_size = config->panel_width * config->panel_height * 3 * LEDSCAPE_MATRIX_OUTPUTS * LEDSCAPE_MATRIX_PANELS;

	ledscape_t * const leds = calloc(1, sizeof(*leds));

	*leds = (ledscape_t) {
		.config		= config_union,
		.pru		= pru,
		.width		= config->leds_width,
		.height		= config->leds_height,
		.ws281x		= pru->data_ram,
		.frame_size	= frame_size,
	};

	*(leds->ws281x) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.num_pixels	= (config->leds_width * 3) * (LEDSCAPE_MATRIX_OUTPUTS * 2),
		.command	= 0,
		.response	= 0,
	};

	ledscape_gpio_init();

	// Initiate the PRU program
	if (!no_pru_init)
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
ledscape_strip_init(
	ledscape_config_t * const config_union,
	int no_pru_init
)
{
	ledscape_strip_config_t * const config = &config_union->strip_config;
	pru_t * const pru = pru_init(0);
	const size_t frame_size = 48 * config->leds_width * 8 * 3;

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
		.config		= config_union,
		.pru		= pru,
		.width		= config->leds_width,
		.height		= 48, // only 48 are supported, config->leds_height,
		.ws281x		= pru->data_ram,
		.frame_size	= frame_size,
	};

	// LED strips, not matrix output
	*(leds->ws281x) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.num_pixels	= config->leds_width, // * leds->height,
		.command	= 0,
		.response	= 0,
	};

	printf("pixels: %d\n", leds->ws281x->num_pixels);

	ledscape_gpio_init();

	// Initiate the PRU program
	if (!no_pru_init)
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
	ledscape_config_t * const config,
	int no_pru_init
)
{
	switch (config->type)
	{
		case LEDSCAPE_MATRIX:
			return ledscape_matrix_init(config, no_pru_init);
		case LEDSCAPE_STRIP:
			return ledscape_strip_init(config, no_pru_init);
		default:
			fprintf(stderr, "unknown config type %d\n", config->type);
			return NULL;
	}
}


void
ledscape_draw(
	ledscape_t * const leds,
	const void * const buffer
)
{
	switch (leds->config->type)
	{
		case LEDSCAPE_MATRIX:
			ledscape_matrix_draw(leds, buffer);
			break;
		case LEDSCAPE_STRIP:
			ledscape_strip_draw(leds, buffer);
			break;
		default:
			fprintf(stderr, "unknown config type %d\n", leds->config->type);
			break;
	}
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
	(void) len;
	uint32_t * start = px;

	//printf("%p => '%s'\n", px, buf);
	for (unsigned i = 0 ; i < sizeof(buf) ; i++)
	{
		char c = buf[i];
		if (!c)
			break;
		if (c == '\n')
		{
			px = start = start + 8 * width;
			continue;
		}

		ledscape_draw_char(px, width, color, c);
		px += 6;
	}
}

/** Default ledscape config */
#define DEFAULT_MATRIX(i) { \
		{ 0*32, i*32, 0 }, \
		{ 1*32, i*32, 0 }, \
		{ 0*32, (i+1)*32, 0 }, \
		{ 1*32, (i+1)*32, 0 }, \
	} \

ledscape_config_t ledscape_matrix_default = {
	.matrix_config = {
		.type		= LEDSCAPE_MATRIX,
		.width		= 64,
		.height		= 64,
		.panel_width	= 32,
		.panel_height 	= 32,
		.leds_width	= 128,
		.leds_height	= 32,
		.panels		= {
			DEFAULT_MATRIX(0),
		},
	},
};
