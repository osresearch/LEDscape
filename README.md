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

Pins used:

according to spreadsheet:
P8 pins:
2.2
2.3
2.5
2.4
1.13
1.12
0.23
0.26
1.15
1.14
0.27
2.1
0.22
1.29

P9 pins:
0.30
1.28
0.31
1.18
1.16
1.19
0.5
0.4
0.13
0.12
0.3
0.2
1.17
0.15
3.21
0.14
3.19
3.17
3.15
3.16
3.14
0.20
3.20
0.7
3.18

------

Sorted by GPIO bank:

0.2
0.3
0.4
0.5
0.7
0.12
0.13
0.14
0.15
0.20
0.22
0.23
0.26
0.27
0.30
0.31

1.12
1.13
1.14
1.15
1.16
1.17
1.18
1.19
1.28
1.29

2.1
2.2
2.3
2.4
2.5

3.14
3.15
3.16
3.17
3.18
3.19
3.20
3.21
