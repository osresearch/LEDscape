/** \file
 * Utility functions.
 */
#ifndef _util_h_
#define _util_h_

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>

#define warn(fmt, ...) \
	do { \
		fprintf(stderr, "%s:%d: " fmt, \
			__func__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define warn_once(fmt, ...) \
	do { \
		static unsigned __warned__; \
		if (__warned__) \
			break; \
		__warned__ = 1; \
		warn(fmt, ## __VA_ARGS__ ); \
	} while (0)

#define die(fmt, ...) \
	do { \
		warn(fmt, ## __VA_ARGS__ ); \
		exit(EXIT_FAILURE); \
	} while(0)


static inline void
hexdump(
	const void * const buf,
	const size_t len
)
{
	const uint8_t * const p = buf;

	for(size_t i = 0 ; i < len ; i++)
	{
		if (i % 16 == 0)
			printf("%s%04zu:", i == 0 ? "": "\n", i);
		printf(" %02x", p[i]);
	}

	printf("\n");
}

#endif
