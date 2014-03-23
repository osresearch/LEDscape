/** \file
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

const int width = 128;
const int height = 128;


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


int
main(
	int argc,
	char ** argv
)
{
	ledscape_t * const leds = ledscape_init(width, height);


	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);
	int scroll_x = 128;
	memset(p, 0x10, width*height*4);

	for (int x = 0 ; x < 128 ; x += 32)
	{
		for (int y = 0 ; y < 128 ; y += 16)
		{
			char buf[32];
			snprintf(buf, sizeof(buf), "%d,%d", x, y);
			font_write(p, 0xFF0000, x, y, buf);
		}
	}

	while (1)
	{
		ledscape_draw(leds, p);
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
