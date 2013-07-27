DANGER!
=======

This code works with the PRU units on the Beagle Bone and can easily
cause *hard crashes*.  It is still being debugged and developed.

Overview
========

The WS281x LED chips are built like shift registers and make for very
easy LED strip construction.  The signals are duty-cycle modulated,
with a 0 measuring 250 ns long and a 1 being 600 ns long, and 1250 ns
between bits.  Since this doesn't map to normal SPI hardware and requires
an 800 KHz bit clock, it is typically handled with a dedicated microcontroller
or DMA hardware on something like the Teensy 3.

However, the TI OMAP in the BeagleBone Black has two programmable
"microcontrollers" built into the CPU that can handle real time
tasks and also access the ARM's memory.  This allows things that
might have been delegated to external devices to be handled without
any additional hardware, and without the overhead of clocking data out
the USB port.
