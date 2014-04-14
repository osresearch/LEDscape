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
	ledscape_matrix_config_t * const config = &ledscape_matrix_default.matrix_config;
	ledscape_t * const leds = ledscape_init(&ledscape_matrix_default);

	printf("init done %d,%d\n", config->width, config->height);
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(config->width*config->height,4);
	int scroll_x = 128;
	memset(p, 0x10, config->width*config->height*4);

	for (int x = 0 ; x < config->width ; x += 32)
	{
		for (int y = 0 ; y < config->height ; y += 16)
		{
			ledscape_printf(
				&p[x + config->width*y],
				config->width,
				0x00FF00, // green
				"x=%d",
				x
			);

			ledscape_printf(
				&p[x + config->width*(y+8)],
				config->width,
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
