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


static game_t boards[6] = {};

static void
randomize(
	game_t * const b,
	int chance
)
{
	for (int y = 0 ; y < WIDTH ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
#if 1
			unsigned live = (rand() % 128 < chance);
			b->board[y][x] = live ? 3 : 0;
#else
			b->board[y][x] = 0;
#endif
		}
	}

}


static void
make_glider(
	game_t * const b
)
{
	//  X
	//   X
 	// XXX
	int px = (rand() % 8) + 10;
	int py = (rand() % 8) + 20;
	b->board[py+0][px+0] = 0;
	b->board[py+0][px+1] = 3;
	b->board[py+0][px+2] = 0;

	b->board[py+1][px+0] = 0;
	b->board[py+1][px+1] = 0;
	b->board[py+1][px+2] = 3;

	b->board[py+2][px+0] = 3;
	b->board[py+2][px+1] = 3;
	b->board[py+2][px+2] = 3;
}


static uint8_t *
_get_edge(
	edge_t * const e,
	int pos
)
{
	game_t * const b = e->board;
	const int edge = e->edge;
	const int neg_pos = WIDTH - pos - 1;

	if (edge == 1)
		return &b->board[0][pos];
	if (edge == 2)
		return &b->board[pos][WIDTH-1];
	if (edge == 3)
		return &b->board[WIDTH-1][pos];
	if (edge == 4)
		return &b->board[pos][0];

	if (edge == -1)
		return &b->board[0][neg_pos];
	if (edge == -2)
		return &b->board[neg_pos][WIDTH-1];
	if (edge == -3)
		return &b->board[WIDTH-1][neg_pos];
	if (edge == -4)
		return &b->board[neg_pos][0];

printf("bad %d,%d\n", edge, pos);
	return NULL;
}


static uint8_t
get_edge(
	edge_t * const e,
	int pos
)
{
	uint8_t * const u = _get_edge(e, pos);
	if (u)
		return (*u) & 1;
	return 0;
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
identify(
	uint8_t * const out,
	int x,
	int y
)
{
	uint32_t b = 0;

	if (x < 32)
	{
		if (y < 32)
			b = 0xFF0000;
		else
		if (y < 64)
			b = 0x0000FF;
		else
		if (y < 96)
			b = 0x00FF00;
		else
			b = 0x411111;
	} else
	if (x < 64)
	{
		if (y < 32)
			b = 0xFF00FF;
		else
		if (y < 64)
			b = 0x00FFFF;
		else
		if (y < 96)
			b = 0xFFFF00;
		else
			b = 0x114111;
	} else {
		b = 0x111141;
	}
		
	out[0] = (b >> 16) & 0xFF;
	out[1] = (b >>  8) & 0xFF;
	out[2] = (b >>  0) & 0xFF;
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
				r += 80;
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
#define SMOOTH_R 3
#define SMOOTH_G 127
#define SMOOTH_B 63
#else
#define SMOOTH_R 1
#define SMOOTH_G 1
#define SMOOTH_B 1
#endif
				r = (r * SMOOTH_R) / (SMOOTH_R+1);
				g = (g * SMOOTH_G) / (SMOOTH_G+1);
				b = (b * SMOOTH_B) / (SMOOTH_B+1);
			}

			if (0 && x == 0 && y == 0)
				identify(pix_ptr, board->px, board->py);
			else
				*pix_ptr = (r << 0) | (g << 8) | (b << 16);
		}
	}
}


static void
check_edge(
	game_t * const boards,
	int i,
	edge_t *edge,
	int this_edge
)
{
	game_t * b = &boards[i];
	game_t * n = edge->board;
	int e = edge->edge;

	// verify that the back link from the remote board is to this
	// edge on this board.
	if (e == 1 && n->top.board == b && n->top.edge == this_edge)
		return;
	if (e == 2 && n->right.board == b && n->right.edge == this_edge)
		return;
	if (e == 3 && n->bottom.board == b && n->bottom.edge == this_edge)
		return;
	if (e == 4 && n->left.board == b && n->left.edge == this_edge)
		return;

	if (e == -1 && n->top.board == b && n->top.edge == -this_edge)
		return;
	if (e == -2 && n->right.board == b && n->right.edge == -this_edge)
		return;
	if (e == -3 && n->bottom.board == b && n->bottom.edge == -this_edge)
		return;
	if (e == -4 && n->left.board == b && n->left.edge == -this_edge)
		return;

	fprintf(stderr, "%d edge %d bad back link?\n", i, this_edge);
	exit(-1);
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

	game_t * const red = &boards[0];
	game_t * const purple = &boards[1];
	game_t * const green = &boards[2];
	game_t * const yellow = &boards[3];
	game_t * const blue = &boards[4];
	game_t * const teal = &boards[5];

	// red
	red->px		= 0;
	red->py		= 0;
	red->top	= (edge_t) { green, 4 };
	red->right	= (edge_t) { purple, 4 };
	red->bottom	= (edge_t) { blue, -3 };
	red->left	= (edge_t) { teal, -3 };

	// purple
	purple->px	= WIDTH;
	purple->py	= 0;
	purple->top	= (edge_t) { green, 3 };
	purple->right	= (edge_t) { yellow, 3 };
	purple->bottom	= (edge_t) { blue, -4 };
	purple->left	= (edge_t) { red, 2 };

	// green
	green->px	= 0;
	green->py	= 2*WIDTH;
	green->top	= (edge_t) { teal, -2 };
	green->right	= (edge_t) { yellow, 4 };
	green->bottom	= (edge_t) { purple, 1 };
	green->left	= (edge_t) { red, 1 };

	// yellow
	yellow->px	= WIDTH;
	yellow->py	= 2*WIDTH;
	yellow->top	= (edge_t) { teal, -1 };
	yellow->right	= (edge_t) { blue, -1 };
	yellow->bottom	= (edge_t) { purple, 2 };
	yellow->left	= (edge_t) { green, 2 };

	// blue
	blue->px	= 0;
	blue->py	= WIDTH;
	blue->top	= (edge_t) { yellow, -2 };
	blue->right	= (edge_t) { teal, 4 };
	blue->bottom	= (edge_t) { red, -3 };
	blue->left	= (edge_t) { purple, -3 };

	// teal
	teal->px	= WIDTH;
	teal->py	= WIDTH;
	teal->top	= (edge_t) { yellow, -1 };
	teal->right	= (edge_t) { green, -1 };
	teal->bottom	= (edge_t) { red, -4 };
	teal->left	= (edge_t) { blue, 2 };

	for (int i = 0 ; i < 6 ; i++)
	{
		game_t * b = &boards[i];
		check_edge(boards, i, &boards[i].top, 1);
		check_edge(boards, i, &boards[i].right, 2);
		check_edge(boards, i, &boards[i].bottom, 3);
		check_edge(boards, i, &boards[i].left, 4);
	}

	srand(getpid());

if (1){
	int which = 5;
	game_t * const b = &boards[which];
	
	b->board[0][4] = 3;
	*_get_edge(&b->top, 4) = 3;

	b->board[6][WIDTH-1] = 3;
	*_get_edge(&b->right, 6) = 3;

	b->board[WIDTH-1][8] = 3;
	*_get_edge(&b->bottom, 8) = 3;

	b->board[8][0] = 3;
	*_get_edge(&b->left, 8) = 3;
}


	while (1)
	{
		if ((i & 0x3FF) == 0)
		{
			printf("randomize\n");
			for (int i = 0 ; i < 6 ; i++)
			{
				randomize(&boards[i], 20);
				//make_glider(&boards[i]);
			}
			//make_glider(&boards[rand() % 6]);
		}

		if (i++ % 4 == 0)
		{
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
