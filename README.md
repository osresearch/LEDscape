![Testing LEDscape](http://farm4.staticflickr.com/3834/9378678019_b706c55635_z.jpg)

DANGER!
=======

This code works with the PRU units on the Beagle Bone and can easily
cause *hard crashes*.  It is still being debugged and developed.
Be careful hot-plugging things into the headers -- it is possible to
damage the pin drivers and cause problems in the ARM, especially if
there are +5V signals involved.


Overview
========

The WS281x LED chips are built like shift registers and make for very
easy LED strip construction.  The signals are duty-cycle modulated,
with a 0 measuring 250 ns long and a 1 being 600 ns long, and 1250 ns
between bits.  Since this doesn't map to normal SPI hardware and requires
an 800 KHz bit clock, it is typically handled with a dedicated microcontroller
or DMA hardware on something like the Teensy 3.

However, the TI AM335x ARM Cortex-A8 in the BeagleBone Black has two
programmable "microcontrollers" built into the CPU that can handle realtime
tasks and also access the ARM's memory.  This allows things that
might have been delegated to external devices to be handled without
any additional hardware, and without the overhead of clocking data out
the USB port.

The frames are stored in memory as a series of 4-byte pixels in the
order GRBA, packed in strip-major order.  This means that it looks
like this in RAM:

	S0P0 S1P0 S2P0 ... S31P0 S0P1 S1P1 ... S31P1 S0P2 S1P2 ... S31P2

This way length of the strip can be variable, although the memory used
will depend on the length of the longest strip.  4 * 32 * longest strip
bytes are required per frame buffer.  The maximum frame rate also depends
on the length of th elongest strip.


API
===

There is a command structure shared in PRU DRAM that holds a pointer
to the current frame buffer, the length in pixels, a command byte and
a response byte.

	typedef struct
	{
		// in the DDR shared with the PRU
		const uintptr_t pixels_dma;

		// Length in pixels of the longest LED strip.
		unsigned size;

		// write 1 to start, 0xFF to abort. will be cleared when started
		volatile unsigned command;

		// will have a non-zero response written when done
		volatile unsigned response;
	} __attribute__((__packed__)) ws281x_command_t;

Once the PRU has cleared the command byte you are free to re-write the
dma address or number of pixels.  You can double buffer like this:

	unsigned frame_id = 0;
	pixel_slice_t * frames[2];
	uintptr_t frames_dma[2];

	while (1)
	{
		render(frames[frame_id]);
		cmd->pixels_dma = frames_dma[frame_id];

		// wait for the previous frame to finish display
		while(!cmd->reponse)
			;

		// Send the start command and wait for the ack
		cmd->command = 1;
		while(cmd->command)
			;

		frame_id = (frame_id+1) % 2;
	}

The 24-bit RGB data to be displayed is laid out with BRGA format,
since that is how it will be translated during the clock out from the PRU.

	typedef struct {
		uint8_t b;
		uint8_t r;
		uint8_t g;
		uint8_t a;
	} __attribute__((__packed__)) pixel_t;

	typedef struct {
		pixel_t strip[32];
	} __attribute__((__packed__)) pixel_slice_t;

LED Strips
==========

* http://www.adafruit.com/products/1138
* http://www.adafruit.com/datasheets/WS2811.pdf
