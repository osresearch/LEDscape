/** \file
 * Play the game of life on the matrix cube.
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


/* LED mappings:

           +--------+
           |        |
           | Yellow |
           |o  3   ^|
           +-------|+
           |   2   ||
           | Green  |
           |o 0x32  |
           +--------+--------+
           |o  0    |o  1    |
           |  red   | purple |
           |0x0    -->       |
  +--------+--------+--------+
  |   5   <--  4    |
  |  Teal  | Blue   |
  |       o|  0x64 o|
  +--------+--------+
 */

#define WIDTH 32
typedef struct
{
	unsigned px, py;
	unsigned rotate; // 0, 1, 2, or 3, viewed from the top

	uint8_t edges[4][2]; // top, right, bottom, left
	uint8_t board[WIDTH][WIDTH];
} game_t;

static game_t boards[6] = {
	{
		// red
		.px = 0, .py = 0, .rotate = 0,
	},
	{
		// purple
		.px = WIDTH, .py = 0, .rotate = 0,
	},
	{
		// green
		.px = 0, .py = WIDTH, .rotate = 3,
	},
	{
		// yellow
		.px = WIDTH, .py = WIDTH, .rotate = 3,
	},
	{
		// blue
		.px = 0, .py = 2*WIDTH, .rotate = 2,
	},
	{
		// teal
		.px = WIDTH, .py = 2*WIDTH, .rotate = 2,
	},
	
};

static void
randomize(
	game_t * const b
)
{
	for (int y = 0 ; y < WIDTH ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
			unsigned live = (rand() % 128 < 20);
			b->board[y][x] = live ? 3 : 0;
		}
	}
}


static void
play_game(
	game_t * const b
)
{
	for (int y = 0 ; y < WIDTH ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
			uint8_t sum = 0;
			const unsigned bx = x == 0;
			const unsigned by = y == 0;
			const unsigned tx = x == WIDTH-1;
			const unsigned ty = y == WIDTH-1;

			if (!bx && !by)
				sum += b->board[y-1][x-1] & 1;
			if (!bx && !ty)
				sum += b->board[y+1][x-1] & 1;

			if (!tx && !by)
				sum += b->board[y-1][x+1] & 1;
			if (!tx && !ty)
				sum += b->board[y+1][x+1] & 1;

			if (!bx)
				sum += b->board[y+0][x-1] & 1;
			if (!tx)
				sum += b->board[y+0][x+1] & 1;

			if (!by)
				sum += b->board[y-1][x+0] & 1;
			if (!ty)
				sum += b->board[y+1][x+0] & 1;


/*
Any live cell with fewer than two live neighbours dies,
as if caused by under-population.
Any live cell with two or three live neighbours lives
on to the next generation.
Any live cell with more than three live neighbours dies,
as if by overcrowding.
Any dead cell with exactly three live neighbours becomes a live cell,
as if by reproduction.
*/
			if (b->board[y][x] & 1)
			{
				// currently live
				if (sum == 2 || sum == 3)
					b->board[y][x] |= 2;
				else
					b->board[y][x] &= ~2;
			} else {
				// currently dead
				if (sum == 3)
					b->board[y][x] |= 2;
				else
					b->board[y][x] &= ~2;
			}
		}
	}
}


static void
copy_to_fb(
	uint32_t * const p,
	const unsigned width,
	const unsigned height,
	game_t * const board
)
{
	for (int y = 0 ; y < WIDTH ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
			uint32_t * const pix_ptr = &p[width*(y+board->py) + x + board->px];
			uint32_t pix = *pix_ptr;
			unsigned r = (pix >>  0) & 0xFF;
			unsigned g = (pix >>  8) & 0xFF;
			unsigned b = (pix >> 16) & 0xFF;

			// copy the new value to the current value
			uint8_t * const sq = &board->board[y][x];
			*sq = (*sq & 2) | (*sq >> 1);

			unsigned live = *sq & 1;
			if (live)
			{
				g += 10;
				if (g > 0xFF)
					g = 0xFF;
			} else {
#define SMOOTH 16
				r = (r * SMOOTH) / (SMOOTH+1);
				g = (g * SMOOTH) / (SMOOTH+1);
				b = (b * SMOOTH) / (SMOOTH+1);
			}

			*pix_ptr = (r << 0) | (g << 8) | (b << 16);
		}
	}
}


int
main(void)
{
	const int width = 128;
	const int height = 128;
	ledscape_t * const leds = ledscape_init(width, height);
	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);

	while (1)
	{
		if ((i & 0x001FF) == 0)
		{
			printf("randomize\n");
		for (int i = 0 ; i < 6 ; i++)
			randomize(&boards[i]);
		}

		if (i++ % 4 == 0)
		{
			printf("i=%x\n", i);
		for (int i = 0 ; i < 6 ; i++)
			play_game(&boards[i]);
		}

		for (int i = 0 ; i < 6 ; i++)
			copy_to_fb(p, width, height, &boards[i]);

		ledscape_draw(leds, p);
		usleep(10000);
	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
