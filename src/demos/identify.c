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


int
main(
	int argc,
	char ** argv
)
{
	int width = 256;
	int height = 32;

	ledscape_config_t * config = &ledscape_matrix_default;
	if (argc > 1)
	{
		config = ledscape_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
	}

	if (config->type == LEDSCAPE_MATRIX)
	{
		config->matrix_config.width = width;
		config->matrix_config.height = height;
	}

	ledscape_t * const leds = ledscape_init(config, 0);

	printf("init done %d,%d\n", width, height);
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);
	int scroll_x = 128;
	memset(p, 0x10, width*height*4);

	int h = 4;
	const uint32_t colors[] = {
		0xFF0000,
		0x00FF00,
		0x0000FF,
		0xFF00FF,
		0x00FFFF,
		0xFFFF00,
	};

	while (1)
	{
		if (h++ == 2*width)
			h = 10;

		for(int y = 0 ; y < height ; y++)
		{
			uint32_t * const row_ptr = &p[width*y];
			const int scale = 63;
			for(int x = 5 ; x < width ; x++)
			{
				uint32_t color = row_ptr[x];
				int r = (color >> 16) & 0xFF;
				int g = (color >>  8) & 0xFF;
				int b = (color >>  0) & 0xFF;
				r = (r * scale) / (scale+1);
				g = (g * scale) / (scale+1);
				b = (b * scale) / (scale+1);
				if (r < 10) r = 10;
				if (g < 10) g = 10;
				if (b < 10) b = 10;
				row_ptr[x] = r << 16 | g << 8 | b << 0;
			}
		}

		for(int y = 0 ; y < height ; y++)
		{
			uint32_t * const row_ptr = &p[width*y];
			uint32_t color = colors[y % 6];
			row_ptr[0] = y & 1 ? 0xFFFFFF : 0x040404;
			row_ptr[1] = y & 2 ? 0xFFFFFF : 0x040404;
			row_ptr[2] = y & 4 ? 0xFFFFFF : 0x040404;
			row_ptr[3] = y & 8 ? 0xFFFFFF : 0x040404;
			row_ptr[4] = y & 16 ? 0xFFFFFF : 0x040404;

			row_ptr[5] = color;
			row_ptr[h/2] = color;
		}

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
