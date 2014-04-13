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

const int width = 256;
const int height = 32;

extern const uint16_t font[][16];

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


#if 0
static ledscape_matrix_config_t ledscape_config =
{
	// frame buffer size
	.width = 256,
	.height = 32,

	// panel size
	.panel_width = 32,
	.panel_height = 16,

	// ledscape matrix output size
	.leds_width = LEDSCAPE_MATRIX_PANELS*32, // could be less
	.leds_height = LEDSCAPE_MATRIX_OUTPUTS*16,

	.panels = {
	[0] = {
		[0] = {  0, 0, 1 },
		[1] = { 16, 0, 2 },
		[2] = { 32, 0, 1 },
		[3] = { 48, 0, 2 },
		[4] = { 64, 0, 1 },
		[5] = { 80, 0, 2 },
		[6] = { 96, 0, 1 },
		[7] = { 112, 0, 2 },
	},
	[1] = {
		[0] = { 128, 0, 1 },
		[1] = { 144, 0, 2 },
		[2] = { 160, 0, 1 },
		[3] = { 176, 0, 2 },
		[4] = { 192, 0, 1 },
		[5] = { 208, 0, 2 },
		[6] = { 224, 0, 1 },
		[7] = { 240, 0, 2 },
	},
	},
};
#endif



int
main(
	int argc,
	char ** argv
)
{
	const int leds_width = 256;
	const int leds_height = 128;
	const int width = 256;
	const int height = 32;
	ledscape_t * const leds
		= ledscape_init(leds_width, leds_height);
	ledscape_matrix_config_t * config = NULL;

	if (argc > 1)
	{
		config = ledscape_matrix_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
		config->width = width;
		config->height = height;
	}


	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height, 4);
	uint32_t * const fb = calloc(leds_width*leds_height,4);
	int scroll_x = 256;

	while (1)
	{
		font_write(p, 0x00FF00, 0, 0, "1.! NYCResistor-Atlantic Pacific");
		font_write(p, 0xFF0000, 11, 0, "!");
		font_write(p, 0x00FF00, 224, 0, "8min");
		
		int end_x = font_write(p, 0xFF4000, scroll_x, 16, argc > 2 ? argv[2] : "");
		if (end_x <= 0)
			scroll_x = width;
		else
			scroll_x--;

		if (config)
		{
			ledscape_matrix_remap(fb, p, config);
			ledscape_draw(leds, fb);
		} else {
			ledscape_draw(leds, p);
		}
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
