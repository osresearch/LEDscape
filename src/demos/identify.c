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

	ledscape_t * const leds = ledscape_init(config, 0);

	//printf("init done %d,%d\n", width, height);
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);
	int scroll_x = 128;
	memset(p, 0x10, width*height*4);

	for (int i = 0 ; i < 8 ; i++)
	{
		for (int j = 0 ; j < 8 ; j++)
		{
			ledscape_printf(
				&p[8+j*32 + width*i*16],
				width,
				0xFF0000, // red
				"%d-%d",
				i,
				j
			);
			ledscape_printf(
				&p[1+j*32 + width*i*16],
				width,
				0x00FF00, // green
				"^"
			);
			ledscape_printf(
				&p[1+j*32 + width*(i*16+8)],
				width,
				0x0000FF, // blue
				"|"
			);
			p[j*32+width*i*16] = 0xFFFF00;
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
