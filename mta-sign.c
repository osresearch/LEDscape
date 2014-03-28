/** \file
 * Test the matrix LCD PRU firmware with a multi-hue rainbow.
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
#include "mta-font.c"

const int width = 256;
const int height = 32;


static int
font_write(
	uint32_t * const buf,
	const uint32_t color,
	const int x0,
	const int y0,
	const char * s
)
{
	int x = x0;
	int y = y0;

	while (1)
	{
		char c = *s++;
		if (!c)
			break;

		if (c == '\n')
		{
			x = x0;
			y += 16 * width;
			continue;
		}

		const uint16_t * ch = font[(uint8_t) c];
		int max_width = 0;

		if (c == ' ' || c == '.')
			max_width = 3;
		else
		for (int h = 0 ; h < 16 ; h++)
		{
			int width = 0;
			uint16_t row = ch[h] >> 2;
			while (row)
			{
				row >>= 1;
				width++;
			}

			if (width > max_width)
				max_width = width;
		}

		// add space after the character
		max_width++;

		for (int h = 0 ; h < 16 ; h++)
		{
			uint16_t row = ch[h] >> 2;
			for (int j = 0 ; j < max_width ; j++, row >>= 1)
			{
				uint32_t pixel_color = (row & 1) ? color : 0;
				int ox = x + j;
/*
				if (x + j >= width || x + j < 0)
					continue;
*/
				if (ox >= width)
					continue;

				// wrap in x
				if (ox < 0)
					ox += width;
		
				if (y + h >= height || y + h < 0)
					continue;

				uint8_t * pix = (uint8_t*) &buf[(y+h)*width + ox];
			       	pix[0] = pixel_color >> 16;
			       	pix[1] = pixel_color >>  8;
			       	pix[2] = pixel_color >>  0;
			}
		}

		x += max_width;
	}

	return x;
}


typedef struct {
	int x;
	int y;
	int rot; // 0 == none, 1 == left, 2 == right, 3 == flip
} ledscape_panel_t;

#define LEDSCAPE_OUTPUTS 8 // number of outputs on the cape
#define LEDSCAPE_PANELS 8 // number of panels chained per output

typedef struct {
	int width;
	int height;
	int panel_width;
	int panel_height;
	int leds_width;
	int leds_height;
	ledscape_panel_t panels[LEDSCAPE_OUTPUTS][LEDSCAPE_PANELS];
} ledscape_config_t;

static ledscape_config_t ledscape_config =
{
	// frame buffer size
	.width = 256,
	.height = 32,

	// panel size
	.panel_width = 32,
	.panel_height = 16,

	// ledscape matrix output size
	.leds_width = LEDSCAPE_PANELS*32, // could be less
	.leds_height = LEDSCAPE_OUTPUTS*16,

	.panels = {
	[0] = {
		[0] = {  0, 0, 2 },
		[1] = { 16, 0, 1 },
		[2] = { 32, 0, 2 },
		[3] = { 48, 0, 1 },
		[4] = { 64, 0, 2 },
		[5] = { 80, 0, 1 },
		[6] = { 96, 0, 2 },
		[7] = { 112, 0, 1 },
	},
	[1] = {
		[7] = { 128, 0, 2 },
		[6] = { 144, 0, 1 },
		[5] = { 160, 0, 2 },
		[4] = { 176, 0, 1 },
		[3] = { 192, 0, 2 },
		[2] = { 208, 0, 1 },
		[1] = { 224, 0, 2 },
		[0] = { 240, 0, 1 },
	},
	},
};


/** Copy a 16x32 region from in to a 32x16 region of out.
 * If rot == 0, rotate -90, else rotate +90.
 */
static void
framebuffer_copy(
	uint32_t * const out,
	const uint32_t * const in,
	const ledscape_config_t * const config,
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


/** With the panels mounted vertically, adjust the mapping.
 * Even panels are rotated -90, odd ones +90.
 * Input framebuffer is 256x32
 */
void
framebuffer_fixup(
	uint32_t * const out,
	const uint32_t * const in,
	const ledscape_config_t * const config
)
{
	for (int i = 0 ; i < LEDSCAPE_OUTPUTS ; i++)
	{
		for (int j = 0 ; j < LEDSCAPE_PANELS ; j++)
		{
			const ledscape_panel_t * const panel
				= &config->panels[i][j];

			int ox = config->panel_width * j;
			int oy = config->panel_height * i;
			printf("%d,%d => %d,%d <= %d,%d,%d\n", i, j, ox, oy, panel->x, panel->y, panel->rot);

			const uint32_t * const ip
				= &in[panel->x + panel->y * config->width];
			uint32_t * const op
				= &out[ox + oy * config->leds_width];
		
			framebuffer_copy(
				op,
				ip,
				config,
				panel->rot
			);
		}
	}
}



int
main(
	int argc,
	char ** argv
)
{
	ledscape_t * const leds
		= ledscape_init(ledscape_config.leds_width, ledscape_config.leds_height);


	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(ledscape_config.width*ledscape_config.height, 4);
	uint32_t * const fb = calloc(ledscape_config.leds_width*ledscape_config.leds_height,4);
	int scroll_x = 256;

	while (1)
	{
		font_write(p, 0x00FF00, 0, 0, "1.! NYCResistor-Atlantic Pacific");
		font_write(p, 0xFF0000, 11, 0, "!");
		font_write(p, 0x00FF00, 224, 0, "8min");
		
		int end_x = font_write(p, 0xFF4000, scroll_x, 16, argc > 1 ? argv[1] : "");
		if (end_x <= 0)
			scroll_x = width;
		else
			scroll_x--;

		framebuffer_fixup(fb, p, &ledscape_config);
		ledscape_draw(leds, fb);
		usleep(20000);

		// wait for the previous frame to finish;
		//const uint32_t response = ledscape_wait(leds);
		const uint32_t response = 0;
		time_t now = time(NULL);
		if (now != last_time)
		{
			printf("%d fps. starting %d previous %"PRIx32"\n",
				i - last_i, i, response);
			last_i = i;
			last_time = now;
		}

	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
