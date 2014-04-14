/** \file
 * Play the game of life on the normal pyramid
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

#define LIFE_R 0xFF
#define LIFE_G 0x35
#define LIFE_B 0x90
#define SMOOTH_R 127
#define SMOOTH_G 20
#define SMOOTH_B 10
#define DELTA_R 10
#define DELTA_G 30
#define DELTA_B 80

#define WIDTH 128
#define HEIGHT 256

typedef struct
{
	uint8_t board[HEIGHT][WIDTH];
} game_t;

static game_t board;


static void
randomize(
	game_t * const b,
	int chance
)
{
	for (int y = 0 ; y < HEIGHT ; y++)
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



static unsigned
get_space(
	game_t * const b,
	int x,
	int y
)
{
	if (x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT)
		return b->board[y][x] & 1;

	return 0;
}


static void
play_game(
	game_t * const b
)
{
	for (int y = 0 ; y < HEIGHT ; y++)
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
	(void) height;

	for (int y = 0 ; y < HEIGHT ; y++)
	{
		for (int x = 0 ; x < WIDTH ; x++)
		{
			uint32_t * const pix_ptr = &p[width*y + x];
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
				r += DELTA_R;
				g += DELTA_G;
				b += DELTA_B;
				if (r > LIFE_R)
					r = LIFE_R;
				if (g > LIFE_G)
					g = LIFE_G;
				if (b > LIFE_B)
					b = LIFE_B;
			} else {
				r = (r * SMOOTH_R) / (SMOOTH_R+1);
				g = (g * SMOOTH_G) / (SMOOTH_G+1);
				b = (b * SMOOTH_B) / (SMOOTH_B+1);
			}

			*pix_ptr = (r << 0) | (g << 8) | (b << 16);
		}
	}
}



int
main(
	int argc,
	const char ** argv
)
{
	const int leds_width = 128;
	const int leds_height = 256;

	ledscape_matrix_config_t * config = NULL;
	if (argc > 1)
	{
		config = ledscape_matrix_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
		config->width = HEIGHT;
		config->height = WIDTH;
	}

	ledscape_t * const leds = ledscape_init(leds_width, leds_height);
	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	uint32_t * const p = calloc(leds_width*leds_height,4);
	uint32_t * const fb = calloc(WIDTH*HEIGHT,4);

	srand(getpid());

	while (1)
	{
		if ((i & 0x7FF) == 0)
		{
			printf("randomize\n");
			randomize(&board, 20);
			//make_glider(&boards[i]);
		}

		if (i++ % 4 == 0)
		{
			play_game(&board);
		}

		copy_to_fb(p, leds_width, leds_height, &board);

		if (config)
		{
			ledscape_matrix_remap(fb, p, config);
			ledscape_draw(leds, fb);
		} else {
			ledscape_draw(leds, p);
		}
		usleep(1000);
	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
