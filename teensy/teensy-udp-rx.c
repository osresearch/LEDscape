/** \file
 *  UDP image packet receiver.
 *
 * Based on the HackRockCity LED Display code:
 * https://github.com/agwn/pyramidTransmitter/blob/master/LEDDisplay.pde
 *
 * Designed to clock data out to USB teensy3 serial devices, which require
 * bitslicing into the raw form for the OctoWS2811 firmware.
 *
 * Since the teensy devices might come and go, this allows them to go into
 * a disconnected state and will attempt to reconnect when the device port
 * re-appears.
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

static int port = 9999;
static unsigned width = 64;
static unsigned height = 210;

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


typedef struct
{
	const char * dev;
	int fd;
	unsigned id;
	int warned;
} teensy_dev_t;

#define MAX_STRIPS 32
#define MAX_PIXELS 256

typedef struct
{
	unsigned id;
	unsigned x_offset;
	teensy_dev_t * dev;
	uint8_t bad[MAX_PIXELS]; // which pixels are to be masked out
} teensy_strip_t;

// \todo read this from a file at run time
static teensy_strip_t strips[MAX_STRIPS];
static unsigned num_strips;


static teensy_dev_t teensy_devs[] = {
	{ .dev = "/dev/ttyACM0", .fd = -1 },
	{ .dev = "/dev/ttyACM1", .fd = -1 },
	{ .dev = "/dev/ttyACM2", .fd = -1 },
	{ .dev = "/dev/ttyACM3", .fd = -1 },
	{ .dev = "/dev/ttyACM4", .fd = -1 },
	{ .dev = "/dev/ttyACM5", .fd = -1 },
	{ .dev = "/dev/ttyACM6", .fd = -1 },
	{ .dev = "/dev/ttyACM7", .fd = -1 },
};


static const unsigned num_devs = sizeof(teensy_devs) / sizeof(*teensy_devs);
	

static teensy_strip_t *
strip_find(
	teensy_dev_t * const dev
)
{
	// Look through our table to find where this one goes
	for (unsigned j = 0 ; j < num_strips ; j++)
	{
		teensy_strip_t * const leds = &strips[j];
		if (leds->id != dev->id)
			continue;

		if (leds->dev != NULL)
		{
			fprintf(stderr, "FATAL: %s: Duplicate ID %u (%s)\n",
				dev->dev, dev->id, leds->dev->dev);
			return NULL;
		}

		leds->dev = dev;
		return leds;
	}

	fprintf(stderr, "FATAL: %s: Unknown ID %u\n", dev->dev, dev->id);
	return NULL;
}


static void
strip_close(
	teensy_strip_t * const strip
)
{
	teensy_dev_t * const dev = strip->dev;
	if (!dev)
		return;

	if (dev->fd >= 0)
		close(dev->fd);

	dev->fd = -1;
	dev->id = 0;
	strip->dev = NULL;
}


static int
teensy_open(
	teensy_dev_t * const dev
)
{
	dev->fd = serial_open(dev->dev);
	if (dev->fd < 0)
	{
		if (errno == ENOENT)
			return 0;

		warn("%s: Unable to open serial port: %s\n",
			dev->dev,
			strerror(errno)
		);

		// Something else went wrong
		goto fail;
	}

	// Find out the serial number of this teensy
	ssize_t rc;
	rc = write_all(dev->fd, "?", 1);
	if (rc < 0)
	{
		if (!dev->warned)
			warn("%s: write failed: %s\n", dev->dev, strerror(errno));
		dev->warned = 1;
		goto fail;
	}

	dev->warned = 0;

	// \todo: Read until a newline or a timeout
	usleep(10000);
	char response[128];
	rc = read(dev->fd, response, sizeof(response)-1);
	if (rc < 0)
	{
		warn("%s: read failed: %s\n", dev->dev, strerror(errno));
		goto fail;
	}

	response[rc] = '\0';
	if (0) printf("read: '%s'\n", response);

	unsigned this_height;
	if (sscanf(response, "%d,%*d,%*d,%d", &this_height, &dev->id) != 2)
	{
		warn("%s: Unable to parse response: '%s'\n", dev->dev, response);
		goto fail;
	}

	printf("%s: ID %d height %d\n", dev->dev, dev->id, this_height);

	if (dev->id == 0)
	{
		warn("%s: dev id == 0.  NOT GOOD\n", dev->dev);
		goto fail;
	}

	if (this_height != height)
	{
		warn("%s: height %d != expected %d\n", dev->dev, this_height, height);
		goto fail;
	}

	teensy_strip_t * const leds = strip_find(dev);
	if (!leds)
		goto fail;

	return 1;

fail:
	dev->id = 0;
	if (dev->fd >= 0)
		close(dev->fd);
	return -1;
}


static void *
reopen_thread(
	void * const arg
)
{
	(void) arg;

	while (1)
	{
		for (unsigned i = 0 ; i < num_devs ; i++)
		{
			teensy_dev_t * const dev = &teensy_devs[i];
			if (dev->id != 0)
				continue;
			teensy_open(dev);
		}

		sleep(1);
	}

	return NULL;
}


/** Config file format;
 *
 */
static void
read_config(
	const char * const config_file
)
{
	FILE * const f = fopen(config_file, "r");
	if (!f)
		die("%s: Unable to open: %s\n", config_file, strerror(errno));

	char line[1024];

	if (fgets(line, sizeof(line), f) == NULL)
		die("%s: Unable to read port num\n", config_file);

	port = atoi(line);

	if (fgets(line, sizeof(line), f) == NULL)
		die("%s: Unable to read dimenstions\n", config_file);

	if (sscanf(line, "%u,%u\n", &width, &height) != 2)
		die("%s: Parse error on image width or port\n", config_file);

	unsigned line_num = 2;

	for (num_strips = 0 ; num_strips < MAX_STRIPS ; num_strips++, line_num++)
	{
		if (fgets(line, sizeof(line), f) == NULL)
			break;

		teensy_strip_t * const strip = &strips[num_strips];
		int offset = 0;
		int rc = sscanf(line, "%u,%u%n",
			&strip->id,
			&strip->x_offset,
			&offset
		);

	        if (rc != 2)
			die("%s:%d: unable to parse rc=%d\n", config_file, line_num, rc);

		char * s = line + offset;
		if (*s == ',')
			s++;

		do {
			char * eol = strchr(s, ',');
			if (eol)
				*eol++ = '\0';
			if (*s == '\0' || *s == '\n')
				break;

			unsigned bad_strip;
			unsigned bad_pixel;

			if (sscanf(s, "%u:%u", &bad_strip, &bad_pixel) != 2)
				die("%s:%d: unable to parse bad pixels '%s'\n", config_file, line_num, s);

			if (bad_strip >= MAX_STRIPS)
				die("%s:%d: bad bad strip %u\n", config_file, line_num, bad_strip);
			if (bad_pixel >= MAX_PIXELS)
				die("%s:%d: bad bad pixel %u\n", config_file, line_num, bad_pixel);
			strip->bad[bad_pixel] |= 1 << bad_strip;
			s = eol;
			printf("%u: bad %u:%u\n", strip->id, bad_strip, bad_pixel);
		} while (s);
	}

}


int
main(
	int argc,
	char ** argv
)
{
	if (argc != 2)
		die("Require config file path\n");
	read_config(argv[1]);

	const int sock = udp_socket(port);
	if (sock < 0)
		die("socket port %d failed: %s\n", port, strerror(errno));

	pthread_t reopen_tid;
	if (pthread_create(&reopen_tid, NULL, reopen_thread, NULL) < 0)
		die("pthread create failed: %s\n", strerror(errno));

	const size_t image_size = width * height * 3;
	const size_t slice_size = height * 8 * 3 + 3;
	uint8_t * const slice = calloc(1, slice_size);

	// largest possible UDP packet
	uint8_t buf[65536];
	if (sizeof(buf) < image_size + 1)
		die("%u x %u too large for UDP\n", width, height);

	fprintf(stderr, "%u x %u, UDP port %u\n", width, height, port);

	const unsigned report_interval = 10;
	unsigned last_report = 0;
	unsigned long delta_sum = 0;
	unsigned frames = 0;

	while (1)
	{
		const ssize_t rlen = recv(sock, buf, sizeof(buf), 0);
		if (rlen < 0)
			die("recv failed: %s\n", strerror(errno));
		warn_once("received %zu bytes\n", rlen);

		if (buf[0] == 2)
		{
			// image type
			printf("image type: %.*s\n",
				(int) rlen - 1,
				&buf[1]
			);
			continue;
		}

		if (buf[0] != 1)
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
		slice[0] = '*';
		slice[1] = 0;
		slice[2] = 1;

		struct timeval start_tv, stop_tv, delta_tv;
		gettimeofday(&start_tv, NULL);

		// Translate the image from packed RGB into sliced 24-bit
		// for each teensy.
		for (unsigned i = 0 ; i < num_strips ; i++)
		{
			teensy_strip_t * const strip = &strips[i];
			teensy_dev_t * const dev = strip->dev;

			// If it hasn't been opened yet, go onto the next one
			if (!dev)
				continue;

			bitslice(
				slice + 3,
				strip->bad,
				buf + 1,
				width,
				height,
				strip->x_offset
			);

			const ssize_t rc
				= write_all(dev->fd, slice, slice_size);

			if ((size_t) rc == slice_size)
				continue;

			if (rc < 0)
				warn("%s: write failed: %s\n",
					dev->dev,
					strerror(errno)
				);
			else
				warn("%s: short write %zu != %zu: %s\n",
					dev->dev,
					rc,
					slice_size,
					strerror(errno)
				);

			strip_close(strip);
		}

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

	return 0;
}
