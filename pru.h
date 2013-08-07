/** \file
 * Simplified interface to the ARM PRU.
 *
 */
#ifndef _pru_h_
#define _pru_h_

#include <stdio.h>
#include <inttypes.h>
#include "util.h"


/** Mapping of the PRU memory spaces.
 *
 * The PRU has a small, fast local data RAM that is mapped into ARM memory,
 * as well as slower access to the DDR RAM of the ARM.
 */
typedef struct
{
	unsigned pru_num;

	void * data_ram; // PRU data ram in ARM space
	size_t data_ram_size; // size in bytes of the PRU's data RAM

	void * ddr; // PRU DMA address (in ARM space)
	uintptr_t ddr_addr; // PRU DMA address (in PRU space)
	size_t ddr_size; // Size in bytes of the shared space
} pru_t;

extern pru_t *
pru_init(
	const unsigned short pru_num
);


extern void
pru_exec(
	pru_t * const pru,
	const char * const program
);


extern void
pru_close(
	pru_t * const pru
);



/** Configure a GPIO pin.
 *
 * Since the device tree won't do it for us, we need to do it via
 * /sys/class/gpio to set the direction and initial value for
 * all of the pins that we use.
 *
 * Direction 0 == in, 1 == out.
 */
extern int
pru_gpio(
	unsigned gpio,
	unsigned pin,
	unsigned direction,
	const unsigned initial_value
);


#endif
