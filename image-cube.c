/** \file
 * Draw six images on the cube faces
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
main(void)
{
	const int width = 128;
	const int height = 128;
	ledscape_t * const leds = ledscape_init(width, height);
	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	uint32_t bits[32][32];
	memset(bits, 0, sizeof(bits));
	ssize_t rc = read(0, bits, sizeof(bits));
	if (rc != sizeof(bits))
	{
		fprintf(stderr, "only read %zu bytes\n", rc);
		return -1;
	}

	uint32_t * const p = calloc(4, width*height);

	for (int x = 0 ; x < 32 ; x++)
	{
		for (int y = 0 ; y < 32 ; y++)
		{
			uint32_t c = bits[x][y];
			uint32_t r = (c >> 16) & 0xFF;
			uint32_t g = (c >>  8) & 0xFF;
			uint32_t b = (c >>  0) & 0xFF;
			r = r;
			g = g;
			b = b;
			c = (r << 0) | (g << 8) | (b << 16);
			printf("%d,%d %08x %d,%d,%d\n", x, y, c, r, g, b);
			p[width*((31-y)+0) + x] = c;
			p[width*((31-y)+0) + x + 32] = c;
			p[width*((31-x)+32) + (31-y)] = c;
			p[width*((31-x)+32) + (31-y) + 32] = c;
			p[width*((31-x)+64) + (31-y)] = c;
			p[width*((31-x)+64) + (31-y) + 32] = c;
		}
	}
			

	while (1)
	{
		ledscape_draw(leds, p);
		usleep(20000);
	}

	//ledscape_close(leds);

	return EXIT_SUCCESS;
}
