/** \file
 * Userspace interface to the WS281x LED strip driver.
 *
 * \todo Package this into a library, possible a Python module.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"
#include "pru.h"


/** Command structure shared with the PRU.
 *
 * This is mapped into the PRU data RAM and points to the
 * frame buffer in the shared DDR segment.
 *
 * Changing this requires changes in ws281x.p
 */
typedef struct
{
	// in the DDR shared with the PRU
	uintptr_t pixels_dma;

	// Length in pixels of the longest LED strip.
	unsigned num_pixels;

	// write 1 to start, 0xFF to abort. will be cleared when started
	volatile unsigned command;

	// will have a non-zero response written when done
	volatile unsigned response;
} __attribute__((__packed__)) ws281x_command_t;


struct ledscape
{
	ws281x_command_t * ws281x;
	pru_t * pru;
	unsigned num_pixels;
	size_t frame_size;
	ledscape_frame_t * frames[2];
};


/** Retrieve one of the two frame buffers. */
ledscape_frame_t *
ledscape_frame(
	ledscape_t * const leds,
	unsigned int frame
)
{
	if (frame >= 2)
		return NULL;

	return (ledscape_frame_t*)((uint8_t*) leds->pru->ddr + leds->frame_size);
}
	

/** Initiate the transfer of a frame to the LED strips */
void
ledscape_draw(
	ledscape_t * const leds,
	unsigned int frame
)
{
	leds->ws281x->pixels_dma = leds->pru->ddr_addr + leds->frame_size;

	// Wait for any current command to have been acknowledged
	while (leds->ws281x->command)
		;

	// Send the start command
	leds->ws281x->command = 1;
}


/** Wait for the current frame to finish transfering to the strips.
 * \returns a token indicating the response code.
 */
uint32_t
ledscape_wait(
	ledscape_t * const leds
)
{
	while (1)
	{
		uint32_t response = leds->ws281x->response;
		if (!response)
			continue;
		leds->ws281x->response = 0;
		return response;
	}
}


ledscape_t *
ledscape_init(
	unsigned num_pixels
)
{
	pru_t * const pru = pru_init(0);
	const size_t frame_size = num_pixels * LEDSCAPE_NUM_STRIPS * 4;

	if (2 *frame_size > pru->ddr_size)
		die("Pixel data needs at least 2 * %zu, only %zu in DDR\n",
			frame_size,
			pru->ddr_size
		);

	ledscape_t * const leds = calloc(1, sizeof(*leds));

	*leds = (ledscape_t) {
		.pru		= pru,
		.num_pixels	= num_pixels,
		.frame_size	= frame_size,
		.ws281x		= pru->data_ram,
	};

	*(leds->ws281x) = (ws281x_command_t) {
		.pixels_dma	= 0, // will be set in draw routine
		.command	= 0,
		.response	= 0,
		.num_pixels	= leds->num_pixels,
	};

	// Initiate the PRU program
	pru_exec(pru, "./ws281x.bin");

	// Watch for a done response that indicates a proper startup
	// \todo timeout if it fails
	while (!leds->ws281x->response)
		;

	return leds;
}


void
ledscape_close(
	ledscape_t * const leds
)
{
	// Signal a halt command
	leds->ws281x->command = 0xFF;
	pru_close(leds->pru);
}


void
ledscape_set_color(
	ledscape_frame_t * const frame,
	uint8_t strip,
	uint8_t pixel,
	uint8_t r,
	uint8_t g,
	uint8_t b
)
{
	ledscape_pixel_t * const p = &frame[pixel].strip[strip];
	p->r = r;
	p->g = g;
	p->b = b;
}
