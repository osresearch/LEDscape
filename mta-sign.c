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

const int leds_width = 128;
const int leds_height = 128;
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
				if (x + j >= width || x + j < 0)
					continue;
				if (y + h >= height || y + h < 0)
					continue;

				uint8_t * pix = (uint8_t*) &buf[(y+h)*width + x + j];
			       	pix[0] = pixel_color >> 16;
			       	pix[1] = pixel_color >>  8;
			       	pix[2] = pixel_color >>  0;
			}
		}

		x += max_width;
	}

	return x;
}

/** Copy a 16x32 region from in to a 32x16 region of out.
 * If rot == 0, rotate -90, else rotate +90.
 */
static void
framebuffer_copy(
	uint32_t * const out,
	const uint32_t * const in,
	int rot
)
{
	for (int x = 0 ; x < 16 ; x++)
	{
		for (int y = 0 ; y < 32 ; y++)
		{
			int ox, oy;
			if (rot)
			{
				// rotate +90 (0,0) => (0,15)
				ox = y;
				oy = 15 - x;
			} else {
				// rotate -90 (0,0) => (31,0)
				ox = 31 - y;
				oy = x;
			}

			out[ox + leds_width*oy] = in[x + width*y];
		}
	}
}


/** With the panels mounted vertically, adjust the mapping.
 * Even panels are rotated -90, odd ones +90.
 * Input framebuffer is 256x32
 * Output framebuffer is 128x64 (actually x128, but we are not using the
 * other half of it).
 */
void
framebuffer_flip(
	uint32_t * const out,
	const uint32_t * const in
)
{
	for (int x = 0, rot=0 ; x < width ; x += 16, rot = !rot)
	{
		int ox = (x*2) % leds_width;
		int oy = (((x*2)) / leds_width) * 16;

		framebuffer_copy(&out[ox + oy * leds_width], &in[x], rot);
	}
}


int
main(
	int argc,
	char ** argv
)
{
	ledscape_t * const leds = ledscape_init(leds_width, leds_height);


	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height, 4);
	uint32_t * const fb = calloc(leds_width*leds_height,4);
	int scroll_x = 128;

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

		framebuffer_flip(fb, p);
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
