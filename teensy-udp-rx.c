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
#include <unistd.h>
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
 *
 * Since some of the pixels might have bad single channels,
 * allow them to be masked out entirely.
 */
void
bitslice(
	uint8_t * const out,
	const uint8_t * in,
	const unsigned width,
	const unsigned y_offset,
	const uint8_t * const bad_pixels
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
					const uint8_t bit_pos = 1 << y;
					if (bad_pixels[x] & bit_pos)
						continue;
					const uint8_t v
						= in[3*(x + y*width) + channel];
					if (v & mask)
						b |= bit_pos;
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


typedef struct
{
	unsigned id;
	unsigned fd;
	unsigned y_offset;
	const char * dev;
	uint8_t bad[256]; // which pixels are to be masked out
} teensy_strip_t;

// \todo read this from a file at run time
static teensy_strip_t strips[] = {
	{ .id = 14401, .y_offset = 0, .bad = { [2] = 1, } },
	{ .id = 14389, .y_offset = 8, .bad = { [3] = 1, [4] = 1, } },
	{ .id = 8987, .y_offset = 16, .bad = { [0] = 0xFF, } },
	{ .id = 8998, .y_offset = 24, .bad = { [1] = 0x80, } },
};

static const unsigned num_strips = sizeof(strips) / sizeof(*strips);
	

static teensy_strip_t * strip_find(
	const char * const dev,
	const int fd,
	const unsigned id
)
{
	// Look through our table to find where this one goes
	for (unsigned j = 0 ; j < num_strips ; j++)
	{
		teensy_strip_t * const leds = &strips[j];
		if (leds->id != id)
			continue;

		if (leds->dev != NULL)
		{
			fprintf(stderr, "FATAL: %s: Duplicate ID %u (%s)\n",
				dev, id, leds->dev);
			return NULL;
		}

		leds->dev = dev;
		leds->fd = fd;
		return leds;
	}

	fprintf(stderr, "FATAL: %s: Unknown ID %u\n", dev, id);
	return NULL;
}


static ssize_t
write_all(
	const int fd,
	const void * const buf_ptr,
	const size_t len
)
{
	const uint8_t * const buf = buf_ptr;
	size_t offset = 0;

	while (offset < len)
	{
		const ssize_t rc = write(fd, buf + offset, len - offset);
		if (rc < 0)
		{
			if (errno == EAGAIN)
				continue;
			return -1;
		}

		if (rc == 0)
			return -1;

		offset += rc;
	}

	return len;
}
		

int
main(
	int argc,
	char ** argv
)
{
	int port = 9999;
	unsigned width = 10;
	unsigned height = 128;

	const unsigned num_devs = argc - 1;
	if (num_devs != num_strips)
		die("Must specify %u serial devices\n", num_strips);

	int failed = 0;
	for (int i = 1 ; i < argc ; i++)
	{
		const char * const dev = argv[i];
		const int fd = serial_open(dev);
		if (fd < 0)
		{
			warn("%s: Unable to open serial port: %s\n",
				dev,
				strerror(errno)
			);
			failed++;
			continue;
		}

		// Find out the serial number of this teensy
		ssize_t rc;
		rc = write(fd, "?", 1);
		if (rc < 0)
		{
			warn("%s: write failed: %s\n", dev, strerror(errno));
			failed++;
			continue;
		}

		usleep(100000);
		char response[128];
		rc = read(fd, response, sizeof(response)-1);
		if (rc < 0)
		{
			warn("%s: read failed: %s\n", dev, strerror(errno));
			failed++;
			continue;
		}

		response[rc] = '\0';
		if (0) printf("read: '%s'\n", response);

		unsigned this_width;
		unsigned this_id;
		if (sscanf(response, "%d,%*d,%*d,%d", &this_width, &this_id) != 2)
		{
			warn("%s: Unable to parse response: '%s'\n", dev, response);
			failed++;
			continue;
		}

		printf("%s: ID %d width %d\n", dev, this_id, this_width);
		if (this_width != width)
		{
			warn("%s: width %d != expected %d\n", dev, this_width, width);
			failed++;
			continue;
		}

		teensy_strip_t * const leds = strip_find(dev, fd, this_id);
		if (!leds)
			failed++;
	}

	if (failed)
		die("FATAL CONFIGURATION ERROR\n");

	printf("%d serial ports\n", num_devs);

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
			warn_once("Unknown image type '%c' (%02x)\n",
				buf[0],
				buf[0]
			);
			continue;
		}

		if ((size_t) rlen != image_size + 1)
		{
			warn_once("WARNING: Received packet %zu bytes, expected %zu\n",
				rlen,
				image_size + 1
			);
		}

		// Header for the frame to the teensy indicating that it
		// is to be drawn immediately.
		slice[0] = '$';
		slice[1] = 0;
		slice[2] = 0;

		// Translate the image from packed RGB into sliced 24-bit
		// for each teensy.
		for (unsigned i = 0 ; i < num_strips ; i++)
		{
			const teensy_strip_t * const strip = &strips[i];

			bitslice(
				slice + 3,
				buf + 1,
				width,
				strip->y_offset,
				strip->bad
			);

			const ssize_t rc
				= write_all(strip->fd, slice, slice_size);

			if (rc < 0)
				die("%s: write failed: %s\n",
					strip->dev,
					strerror(errno)
				);

			if ((size_t) rc != slice_size)
				die("%s: short write %zu != %zu: %s\n",
					strip->dev,
					rc,
					slice_size,
					strerror(errno)
				);
		}
	}

	return 0;
}
