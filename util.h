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

#define die(fmt, ...) \
	do { \
		fprintf(stderr, "%s:%d: " fmt, \
			__func__, __LINE__, ## __VA_ARGS__); \
		exit(EXIT_FAILURE); \
	} while (0)


#endif
