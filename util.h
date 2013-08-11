/** \file
 * Utility functions.
 */
#ifndef _util_h_
#define _util_h_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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


extern void
hexdump(
	FILE * const outfile,
	const void * const buf,
	const size_t len
);


extern int
serial_open(
	const char * const dev
);


/** Write all the bytes to a fd, even if there is a brief interruption.
 * \return number of bytes written or -1 on any fatal error.
 */
extern ssize_t
write_all(
	const int fd,
	const void * const buf_ptr,
	const size_t len
);

#endif
