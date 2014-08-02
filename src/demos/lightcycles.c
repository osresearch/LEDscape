/** \file
 * Lightcycles style game on the cylindrical megascroller.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>

#define WIDTH 512
#define HEIGHT 64
#define MAX_PLAYERS 4

uint32_t board[HEIGHT][WIDTH];

typedef struct {
	int x;
	int y;
	int dir;
	uint32_t color;
	int alive;
} player_t;

player_t players[MAX_PLAYERS];


int
player_update(
	player_t * const player
)
{
	if (!player->alive)
		return 0;

	switch (player->dir)
	{
	case 0:
		player->x = (player->x + 1 + WIDTH) % WIDTH;
		break;
	case 1:
		player->y = (player->y + 1 + HEIGHT) % HEIGHT;
		break;
	case 2:
		player->x = (player->x - 1 + WIDTH) % WIDTH;
		break;
	case 3:
		player->y = (player->y - 1 + HEIGHT) % HEIGHT;
		break;
	default:
		player->dir = 0;
		break;
	}

	// check for collision
	if (board[player->y][player->x] != 0)
	{
		// dead!
		printf("player %08x died at %d,%d\n",
			player->color, player->x, player->y);
		board[player->y][player->x] = 0xFFFFFF;
		player->alive = 0;
	} else {
		// ok!
		board[player->y][player->x] = player->color;
	}

	return player->alive;
}


static const uint32_t palette[] = 
{
	0xFF0000,
	0x00FF00,
	0x0000FF,
	0xFF00FF,
	0xFFFF00,
};



static void
fill(
	uint32_t color
)
{
	for (int y = 0 ; y < HEIGHT ; y++)
		for (int x = 0 ; x < WIDTH ; x++)
			board[y][x] = color;
}


void
new_game(
	int num_players
)
{
	fill(0);

	for (int i = 0 ; i < MAX_PLAYERS ; i++)
	{
		player_t * const player = &players[i];
		player->alive = i < num_players ? 1 : 0;
		player->x = 0; // rand() % WIDTH;
		player->y = rand() % HEIGHT;
		player->dir = 0; //(rand() % 2) ? 0 : 2;
		player->color = palette[i];
	}
}

static int
tcp_socket(
	const int port
)
{
	const int sock = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};

	if (sock < 0)
		return -1;
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		return -1;

	int max_size = 65536;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &max_size, sizeof(max_size));


	return sock;
}


static
void rgb32_to_rgb24_and_decay(
	uint8_t * const out,
	uint32_t * const in,
	const int width,
	const int height
)
{
	for (int y = 0 ; y < height ; y++)
	{
		for (int x = 0 ; x < width ; x++)
		{
			uint8_t * const pix = out + 3*(x + y*width);
			uint32_t * const in_pix = &in[x + y*width];
			const uint32_t c = *in_pix;

			uint8_t r = pix[0] = (c >> 16) & 0xFF;
			uint8_t g = pix[1] = (c >>  8) & 0xFF;
			uint8_t b = pix[2] = (c >>  0) & 0xFF;

			if (r) r--;
			if (g) g--;
			if (b) b--;

			*in_pix = (r << 16) | (g << 8) | (b << 0);
		}
	}
}


static void
send_game(
	const int sock
)
{
	// copy the game from RGBx to RGB and split into two packets
	uint8_t pkt[65536];

	pkt[0] = 0;
	rgb32_to_rgb24_and_decay(&pkt[1], &board[0][0], WIDTH, HEIGHT/2);
	if (send(sock, pkt, 1 + (WIDTH*HEIGHT/2) * 3, 0) < 0)
	{
		perror("send");
		exit(EXIT_FAILURE);
	}

	pkt[0] = 1;
	rgb32_to_rgb24_and_decay(&pkt[1], &board[HEIGHT/2][0], WIDTH, HEIGHT/2);
	send(sock, pkt, 1 + (WIDTH*HEIGHT/2) * 3, 0);
}


static struct termios ttystate, ttysave;

static void
tty_raw(void)
{
	//get the terminal state
	tcgetattr(STDIN_FILENO, &ttystate);
	ttysave = ttystate;

	//turn off canonical mode and echo
	ttystate.c_lflag &= ~(ICANON | ECHO);
	//minimum of number input read.
	ttystate.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

static void
tty_reset(void)
{
	ttystate.c_lflag |= ICANON | ECHO;
	tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}


// don't allow a turn backwards into yourself
static void
set_dir(
	player_t * const player,
	int new_dir
)
{
	if (new_dir == 0 && player->dir == 2)
		return;
	if (new_dir == 1 && player->dir == 3)
		return;
	if (new_dir == 2 && player->dir == 0)
		return;
	if (new_dir == 3 && player->dir == 1)
		return;

	player->dir = new_dir;
}


static void
bomb(
	player_t * const player
)
{
	// nothing yet
	(void) player;
}


	
static void *
read_thread(
	void * arg
)
{
	(void) arg;
	tty_raw();
	atexit(tty_reset);

	while (1)
	{
		char c;
		ssize_t rc = read(STDIN_FILENO, &c, 1);
		if (rc < 0)
		{
			if (rc == EINTR || rc == EAGAIN)
				continue;
			perror("stdin");
			break;
		}
		if (rc == 0)
			continue;

		switch (c)
		{
		case 'h': set_dir(&players[0], 2); break;
		case 'j': set_dir(&players[0], 1); break;
		case 'k': set_dir(&players[0], 3); break;
		case 'l': set_dir(&players[0], 0); break;
		case ';': bomb(&players[0]); break;

		case 'a': set_dir(&players[1], 2); break;
		case 's': set_dir(&players[1], 1); break;
		case 'w': set_dir(&players[1], 3); break;
		case 'd': set_dir(&players[1], 0); break;
		case 'f': bomb(&players[1]); break;

		default: break;
		}
	}

	return NULL;
}


int main(void)
{
	int port = 9999;
	const char * host = "192.168.1.119";

	const int sock = tcp_socket(0);
	struct sockaddr_in dest = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr = { inet_addr(host) },
	};
	if (connect(sock, (void*) &dest, sizeof(dest)) < 0)
	{
		perror(host);
		return EXIT_FAILURE;
	}

	pthread_t thread_id;
	if (pthread_create(&thread_id, NULL, read_thread, NULL) < 0)
	{
		perror("pthread");
		return EXIT_FAILURE;
	}


	int num_players = 2;

	while (1)
	{
		printf("new game!\n");
		new_game(num_players);

		while (1)
		{
			send_game(sock);

			int alive_count = 0;
			for (int i = 0 ; i < num_players ; i++)
				alive_count += player_update(&players[i]);

			if (alive_count == 1)
				break;
			usleep(5000);
		}

		// one player won!  figure out who
		player_t * winner = NULL;
		for (int i = 0 ; i < num_players ; i++)
		{
			player_t * const player = &players[i];
			if (!player->alive)
				continue;
			winner = player;
			break;
		}

		if (winner == NULL)
		{
			printf("no one won\n");
			continue;
		}

		printf("%08x won\n", winner->color);

		// flash the screen
		for (int i = 0 ; i < 3 ; i++)
		{
			fill(winner->color);
			send_game(sock);
			usleep(250000);

			fill(0);
			send_game(sock);
			usleep(250000);

			fill(winner->color);
			send_game(sock);
			usleep(250000);

			fill(0);
			send_game(sock);
			usleep(500000);
		}
	}
}
