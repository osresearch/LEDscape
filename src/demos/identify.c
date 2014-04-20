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
	const int width = 256;
	const int height = 128;

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

	ledscape_t * const leds = ledscape_init(config);

	//printf("init done %d,%d\n", width, height);
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);
	int scroll_x = 128;
	memset(p, 0x10, width*height*4);

	for (int x = 0 ; x < width ; x += 32)
	{
		for (int y = 0 ; y < height ; y += 16)
		{
			ledscape_printf(
				&p[x + width*y],
				width,
				0x00FF00, // green
				"x=%d",
				x
			);

			ledscape_printf(
				&p[x + width*(y+8)],
				width,
				0xFF00FF, // red
				"y=%d",
				y
			);
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
