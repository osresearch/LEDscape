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
#include <sys/types.h>
#include <sys/socket.h>
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


int
main(
	int argc,
	char ** argv
)
{
	int port = 9999;
	unsigned width = 10;
	unsigned height = 8;

	const int sock = socket(AF_INET, SOCK_DGRAM, 0);
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(port),
		.sin_addr.s_addr = INADDR_ANY,
	};

	if (sock < 0)
		die("socket failed: %s\n", strerror(errno));
	if (bind(sock, (const struct sockaddr*) &addr, sizeof(addr)) < 0)
		die("bind port %d failed: %s\n", port, strerror(errno));

	const size_t slice_size = width * height * 3 + 3;
	uint8_t * const slice = calloc(1, slice_size);

	uint8_t buf[65536];

	while (1)
	{
		const ssize_t rc = recv(sock, buf, sizeof(buf), 0);
		if (rc < 0)
			die("recv failed: %s\n", strerror(errno));

		if (buf[0] == '2')
		{
			// image type
			printf("image type: %.*s\n",
				(int) rc - 1,
				&buf[1]
			);
			continue;
		}

		// Header for the frame to the teensy indicating that it
		// is to be drawn immediately.
		slice[0] = '$';
		slice[1] = 0;
		slice[2] = 0;

		// Translate the image from packed RGB into sliced 24-bit
		// for each teensy.
		for (unsigned y_offset = 0 ; y_offset < height ; y_offset += 8)
		{
			bitslice(slice+3, buf+1, width, y_offset);
			write(1, slice, slice_size);
		}
	}

	return 0;
}
