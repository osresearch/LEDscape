// \file
 //* WS821x LED strip driver for the BeagleBone Black.
 //*
 //* Drives up to 32 strips using the PRU hardware.  The ARM writes
 //* rendered frames into shared DDR memory and sets a flag to indicate
 //* how many pixels wide the image is.  The PRU then bit bangs the signal
 //* out the 32 GPIO pins and sets a done flag.
 //*
 //* To stop, the ARM can write a 0xFFFFFFFF to the width, which will
 //* cause the PRU code to exit.
 //*
 //* At 800 KHz:
 //*  0 is 0.25 usec high, 1 usec low
 //*  1 is 0.60 usec high, 0.65 usec low
 //*  Reset is 50 usec
 //*
 //* So to clock this out:
 //*  ____
 //* |  | |______|
 //* 0  250 600  1250 offset
 //*    250 350   625 delta
 //*
 //* It would be better to do:
 //*
 //* RRRR....
 //* GGGG....
 //* BBBB....
 //* RRRR....
 //* GGGG....
 //* BBBB....
 //* 
 //*/
.origin 0
.entrypoint START

#include "ws821x.hp"

#define GPIO1 0x4804c000
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194

#define WS821X_ENABLE (0x100)
#define DMX_CHANNELS (0x101)
#define DMX_PIN (0x102)

// Sleep a given number of nanoseconds with 10 ns resolution
.macro SLEEPNS
.mparam ns,inst,lab
    MOV r7, (ns/10)-1-inst
lab:
    SUB r7, r7, 1
    QBNE lab, r7, 0
.endm

.macro BRING_LOW
    MOV r6, GPIO1 | GPIO_CLEARDATAOUT
    SBBO r4, r6, 0, 4
.endm

.macro BRING_HIGH
    MOV r6, GPIO1 | GPIO_SETDATAOUT
    SBBO r4, r6, 0, 4
.endm

START:
    // Enable OCP master port
    // clear the STANDBY_INIT bit in the SYSCFG register,
    // otherwise the PRU will not be able to write outside the
    // PRU memory space and to the BeagleBon's pins.
    LBCO	r0, C4, 4, 4
    CLR		r0, r0, 4
    SBCO	r0, C4, 4, 4

    // Configure the programmable pointer register for PRU0 by setting
    // c28_pointer[15:0] field to 0x0120.  This will make C28 point to
    // 0x00012000 (PRU shared RAM).
    MOV		r0, 0x00000120
    MOV		r1, CTPPR_0
    ST32	r0, r1

    // Configure the programmable pointer register for PRU0 by setting
    // c31_pointer[15:0] field to 0x0010.  This will make C31 point to
    // 0x80001000 (DDR memory).
    MOV		r0, 0x00100000
    MOV		r1, CTPPR_1
    ST32	r0, r1
	

    // Wait for the start condition from the main program to indicate
    // that we have a rendered frame ready to clock out.  This also
    // handles the exit case if an invalid value is written to the start
    // start positoin.
_LOOP:
    // Load the pointer to the buffer from PRU DRAM into r0 and the
    // length (in 32-bit words) into r1.
    LBCO      r0, CONST_PRUDRAM, 0, 8

    // Wait for a non-zero length
    QBEQ _LOOP, r1, #0
    // Length of 0xFFFF is the signal to exit
    QBEQ EXIT, r1, #0xFF

    // Store a zero in the length so that they know that we have started
    MOV r2, #0
    SBCO r2, CONST_PRUDRAM, 4, 4

	// We will use these all the time
	MOV r6, GPIO1 | GPIO_SETDATAOUT
	MOV r7, GPIO1 | GPIO_CLEARDATAOUT

    // Clock out the bits!
WORD_LOOP:
	// Load 64-bits of data into r3 and r4
	LBBO r3, r0, 0, 8
	MOV r4, r3

	// for i = 0 to 31
	MOV r2, #0
BIT_START_LOOP:
		// set all the bits high
		SBBO r2, r6, 0, 4
		ADD r2, r2, 1
		QBLT BIT_START_LOOP, r2, #32

	// wait for the zero time
	SLEEPNS 250, 1, wait_zero_time

	// For all the bits that are zero, turn them off now
	// This destroys the bits in r3
	MOV r2, #0
BIT_ZERO_LOOP:
		QBBS non_zero_bit, r3, 1
		SBBO r2, r7, 0, 4 // bring the output pin to 0
		QBA next_zero_bit
non_zero_bit:
		SBBO r2, r6, 0, 4 // leave the output pin at 1
		QBA next_zero_bit
next_zero_bit:
		LSR r3, r3, 1
		ADD r2, r2, 1
		QBLT BIT_ZERO_LOOP, r2, #32
	
	SLEEPNS 350, 1, wait_one_time

	// Turn all the bits off
	MOV r2, #0
BIT_ONE_LOOP:
		SBBO r2, r7, 0, 4 // zero the bit
		ADD r2, r2, 1
		QBLT BIT_ONE_LOOP, r2, #32

	SLEEPNS 625, 1, wait_end_time

	// Increment our data pointer and decrement our length
	ADD r0, r0, #4
	SUB r1, r1, #1
	QBGT WORD_LOOP, r1, #0

    // Write out that we are done!

    // Go back to waiting for the next frame buffer
    QBA _LOOP

EXIT:
#ifdef AM33XX
    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU0_ARM_INTERRUPT
#endif

    HALT
