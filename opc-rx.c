/** \file
 *  OPC image packet receiver.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "util.h"
#include "bitslice.h"
#include "ledscape.h"

typedef struct
{
	uint8_t channel;
	uint8_t command;
	uint8_t len_hi;
	uint8_t len_lo;
} opc_cmd_t;

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
	if (listen(sock, 5) == -1)
		return -1;

	return sock;
}


int
main(
	int argc,
	char ** argv
)
{
	const int port = 7890;
	const int sock = tcp_socket(port);
	if (sock < 0)
		die("socket port %d failed: %s\n", port, strerror(errno));

	const unsigned width = 32;
	const unsigned height = 8;
	const size_t image_size = width * height * 3;

	// largest possible UDP packet
	uint8_t buf[65536];
	if (sizeof(buf) < image_size + 1)
		die("%u x %u too large for UDP\n", width, height);

	fprintf(stderr, "%u x %u, TCP port %u\n", width, height, port);

	ledscape_t * const leds = ledscape_init(width, height);

	const unsigned report_interval = 10;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;

	uint32_t * const fb = calloc(width*height,4);

	int fd;
	while ((fd = accept(sock, NULL, NULL)) >= 0)
	{
	while(1)
	{
		opc_cmd_t cmd;
		ssize_t rlen = read(fd, &cmd, sizeof(cmd));
		if (rlen < 0)
			die("recv failed: %s\n", strerror(errno));
		if (rlen == 0)
		{
			close(fd);
			break;
		}

		const size_t cmd_len = cmd.len_hi << 8 | cmd.len_lo;
		warn("received %zu bytes: %d %zu\n", rlen, cmd.command, cmd_len);

		size_t offset = 0;
		while (offset < cmd_len)
		{
			rlen = read(fd, buf + offset, cmd_len - offset);
			if (rlen < 0)
				die("recv failed: %s\n", strerror(errno));
			if (rlen == 0)
				break;
			offset += rlen;
		}
			
		if (cmd.command != 0)
			continue;

		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		const unsigned frame_num = 0;

		for (unsigned y = 0 ; y < height ; y++)
		{
			for (unsigned x = 0 ; x < width ; x++)
			{
				uint8_t * const out = (void*) &fb[x + y*width];
				const uint8_t * const in = &buf[3*(x + y*width)];
				out[0] = in[0];
				out[1] = in[1];
				out[2] = in[2];
			}
		}

		ledscape_draw(leds, fb);

		gettimeofday(&stop_tv, NULL);
		timersub(&stop_tv, &start_tv, &delta_tv);

		frames++;
		delta_sum += delta_tv.tv_usec;
		if (stop_tv.tv_sec - last_report < report_interval)
			continue;
		last_report = stop_tv.tv_sec;

		const unsigned delta_avg = delta_sum / frames;
		printf("%6u usec avg, max %.2f fps, actual %.2f fps (over %u frames)\n",
			delta_avg,
			report_interval * 1.0e6 / delta_avg,
			frames * 1.0 / report_interval,
			frames
		);

		frames = delta_sum = 0;
	}
	}

	return 0;
}
