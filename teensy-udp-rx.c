/** \file
 *  UDP image packet receiver.
 *
 * Based on the HackRockCity LED Display code:
 * https://github.com/agwn/pyramidTransmitter/blob/master/LEDDisplay.pde
 *
 * Designed to clock data out to USB teensy3 serial devices, which require
 * bitslicing into the raw form for the OctoWS2811 firmware.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
#include "util.h"


/** Prepare one Teensy3 worth of image data.
 * Each Teensy handles 8 rows of data and needs the bits sliced into
 * each 8 rows.
 */
void
bitslice(
	uint8_t * const out,
	const uint8_t * in,
	const unsigned width,
	const unsigned y_offset
)
{
	// Reorder from RGB in the input to GRB in the output
	static const uint8_t channel_map[] = { 1, 0, 2 };

	// Skip to the starting row that we're processing
	in += y_offset * width * 3;

	for(unsigned x=0 ; x < width ; x++)
	{
		for (unsigned channel = 0 ; channel < 3 ; channel++)
		{
			const uint8_t mapped_channel
				= channel_map[channel];

			for (unsigned bit_num = 0 ; bit_num < 8 ; bit_num++)
			{
				uint8_t b = 0;
				const uint8_t mask = 1 << (7 - bit_num);

				for(unsigned y = 0 ; y < 8 ; y++)
				{
					const uint8_t v
						= in[3*(x + y*width) + channel];
					if (v & mask)
						b |= 1 << y;
				}

				out[24*x + 8*mapped_channel + bit_num] = b;
			}
		}
	}
}


static int
udp_socket(
	const int port
)
{
	const int sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};

	if (sock < 0)
		return -1;
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		return -1;

	return sock;
}


static int
serial_open(
	const char * const dev
)
{
	const int fd = open(dev, O_RDWR | O_NONBLOCK | O_NOCTTY, 0666);
	if (fd < 0)
		return -1;

	// Disable modem control signals
	struct termios attr;
	tcgetattr(fd, &attr);
	attr.c_cflag |= CLOCAL | CREAD;
	tcsetattr(fd, TCSANOW, &attr);

	return fd;
}



int
main(
	int argc,
	char ** argv
)
{
	int port = 9999;
	unsigned width = 10;
	unsigned height = 32;

	const unsigned num_fds = argc - 1;
	if (argc <= 1)
		die("At least one serial port must be specified\n");

	int fds[num_fds];
	const char * dev_names[num_fds];

	for (int i = 1 ; i < argc ; i++)
	{
		const char * const dev = argv[i];
		const int fd = serial_open(dev);
		if (fd < 0)
			die("%s: Unable to open serial port: %s\n",
				dev,
				strerror(errno)
			);
		fds[i-1] = fd;
		dev_names[i-1] = dev;
	}

	printf("%d serial ports\n", num_fds);
	if (num_fds * 8 != height)
		fprintf(stderr, "WARNING: %d ports == %d rows != image height %d\n",
			num_fds,
			num_fds * 8,
			height
		);

	const int sock = udp_socket(port);
	if (sock < 0)
		die("socket port %d failed: %s\n", port, strerror(errno));

	const size_t image_size = width * height * 3;
	const size_t slice_size = width * 8 * 3 + 3;
	uint8_t * const slice = calloc(1, slice_size);

	uint8_t buf[65536];

	while (1)
	{
		const ssize_t rlen = recv(sock, buf, sizeof(buf), 0);
		if (rlen < 0)
			die("recv failed: %s\n", strerror(errno));

		if (buf[0] == '2')
		{
			// image type
			printf("image type: %.*s\n",
				(int) rlen - 1,
				&buf[1]
			);
			continue;
		}

		if (buf[0] != '1')
		{
			// What is it?
			fprintf(stderr, "Unknown image type '%c' (%02x)\n",
				buf[0],
				buf[0]
			);
			continue;
		}

		if ((size_t) rlen != width * height * 3 + 1)
		{
			fprintf(stderr, "WARNING: Received packet %zu bytes, expected %zu\n",
				rlen,
				image_size
			);
		}

		// Header for the frame to the teensy indicating that it
		// is to be drawn immediately.
		slice[0] = '$';
		slice[1] = 0;
		slice[2] = 0;

		// Translate the image from packed RGB into sliced 24-bit
		// for each teensy.
		for (unsigned i = 0 ; i < num_fds ; i++)
		{
			const unsigned y_offset = i * 8;
			bitslice(slice+3, buf+1, width, y_offset);

			ssize_t rc = write(fds[i], slice, slice_size);
			if (rc < 0)
				die("%s: write failed: %s\n",
					dev_names[i],
					strerror(errno)
				);
			if ((size_t) rc != slice_size)
				die("%s: short write %zu != %zu: %s\n",
					dev_names[i],
					rc,
					slice_size,
					strerror(errno)
				);
		}
	}

	return 0;
}
