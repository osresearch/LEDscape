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
typedef struct game game_t;
typedef struct {
	game_t * board;
	int edge;
} edge_t;

struct game
{
	unsigned px, py;

	edge_t top;
	edge_t bottom;
	edge_t left;
	edge_t right;

	uint8_t board[WIDTH][WIDTH];
};


static game_t boards[6] = {
	{
		// red
		.px = 0, .py = 0,
	},
	{
		// purple
		.px = WIDTH, .py = 0,
	},
	{
		// green
		.px = 0, .py = WIDTH,
	},
	{
		// yellow
		.px = WIDTH, .py = WIDTH,
	},
	{
		// blue
		.px = 0, .py = 2*WIDTH,
	},
	{
		// teal
		.px = WIDTH, .py = 2*WIDTH,
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


static unsigned
get_edge(
	edge_t * const e,
	int pos
)
{
	game_t * const b = e->board;
	const int edge = e->edge;

	if (edge == 1)
		return b->board[0][pos] & 1;
	if (edge == 2)
		return b->board[pos][WIDTH-1] & 1;
	if (edge == 3)
		return b->board[WIDTH-1][pos] & 1;
	if (edge == 4)
		return b->board[pos][0] & 1;

	if (edge == -1)
		return b->board[0][WIDTH-pos-1] & 1;
	if (edge == -2)
		return b->board[WIDTH-pos-1][WIDTH-1] & 1;
	if (edge == -3)
		return b->board[WIDTH-1][WIDTH-pos-1] & 1;
	if (edge == -4)
		return b->board[WIDTH-pos-1][0] & 1;

printf("bad %d,%d\n", edge, pos);
	return 9;
}


static unsigned
get_space(
	game_t * const b,
	int x,
	int y
)
{
	if (x >= 0 && y >= 0 && x < WIDTH && y < WIDTH)
		return b->board[y][x] & 1;

	// don't deal with diagonal connections
	if (x < 0 && y < 0)
		return 0;
	if (x >= WIDTH && y >= WIDTH)
		return 0;

	// Check for the four cardinal ones
	if (y < 0)
		return get_edge(&b->top, x);
	if (y >= WIDTH)
		return get_edge(&b->bottom, x);
	if (x < 0)
		return get_edge(&b->left, y);
	if (x >= WIDTH)
		return get_edge(&b->right, y);

	// huh?
printf("bad %d,%d\n", x, y);
	return 9;
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

			sum += get_space(b, x-1, y-1);
			sum += get_space(b, x-1, y  );
			sum += get_space(b, x-1, y+1);

			sum += get_space(b, x  , y-1);
			sum += get_space(b, x  , y+1);

			sum += get_space(b, x+1, y-1);
			sum += get_space(b, x+1, y  );
			sum += get_space(b, x+1, y+1);


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
				r += 10;
				g += 5;
				b += 30;
				if (r > 0xFF)
					r = 0xFF;
				if (g > 0xFF)
					g = 0xFF;
				if (b > 0xFF)
					b = 0xFF;
			} else {
#if 1
#define SMOOTH_R 7
#define SMOOTH_G 63
#define SMOOTH_B 15
#else
#define SMOOTH_R 1
#define SMOOTH_G 1
#define SMOOTH_B 1
#endif
				r = (r * SMOOTH_R) / (SMOOTH_R+1);
				g = (g * SMOOTH_G) / (SMOOTH_G+1);
				b = (b * SMOOTH_B) / (SMOOTH_B+1);
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

	boards[0].top		= (edge_t) { &boards[2], 4 };
	boards[0].right		= (edge_t) { &boards[1], 4 };
	boards[0].bottom	= (edge_t) { &boards[4], -3 };
	boards[0].left		= (edge_t) { &boards[5], -3 };

	boards[1].top		= (edge_t) { &boards[2], 4 };
	boards[1].right		= (edge_t) { &boards[3], 4 };
	boards[1].bottom	= (edge_t) { &boards[4], -4 };
	boards[1].left		= (edge_t) { &boards[0], 2 };

	boards[2].top		= (edge_t) { &boards[5], 2 };
	boards[2].right		= (edge_t) { &boards[3], 4 };
	boards[2].bottom	= (edge_t) { &boards[1], 1 };
	boards[2].left		= (edge_t) { &boards[0], 1 };

	boards[3].top		= (edge_t) { &boards[5], 1 };
	boards[3].right		= (edge_t) { &boards[4], -1 };
	boards[3].bottom	= (edge_t) { &boards[1], 2 };
	boards[3].left		= (edge_t) { &boards[2], 2 };

	boards[4].top		= (edge_t) { &boards[3], -2 };
	boards[4].right		= (edge_t) { &boards[5], 4 };
	boards[4].bottom	= (edge_t) { &boards[0], -2 };
	boards[4].left		= (edge_t) { &boards[1], 3 };

	boards[5].top		= (edge_t) { &boards[3], -1 };
	boards[5].right		= (edge_t) { &boards[2], 1 };
	boards[5].bottom	= (edge_t) { &boards[0], -4 };
	boards[5].left		= (edge_t) { &boards[1], 2 };


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
