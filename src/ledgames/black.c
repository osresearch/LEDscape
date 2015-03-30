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

void drawpixel(uint32_t *pixels, uint8_t x, uint8_t y, uint32_t color, uint8_t flip) {
	uint32_t pixelnum = ((flip ? (64 - y) : y) * 64) + (flip ? (64 - x ) : x);
	pixels[pixelnum] = color;
}

void render_game(uint32_t *pixels) {
	for (int y_clear = 0; y_clear < 64; y_clear++) {
		for (int x_clear = 0; x_clear < 64; x_clear++) {
			drawpixel(pixels, x_clear, y_clear, 0x00000000, 0);
		}
	}
}

int
	main(
		int argc,
const char ** argv
	)
{
	int width = 64;
	int height = 64;

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

	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);

	render_game(p);

	i++;
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

	printf("init done\n");
	ledscape_close(leds);

	return EXIT_SUCCESS;
}
