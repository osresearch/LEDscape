// \file
 //* WS281x LED strip driver for the BeagleBone Black.
 //*
 //* Drives up to 32 strips using the PRU hardware.  The ARM writes
 //* rendered frames into shared DDR memory and sets a flag to indicate
 //* how many pixels wide the image is.  The PRU then bit bangs the signal
 //* out the 32 GPIO pins and sets a done flag.
 //*
 //* To stop, the ARM can write a 0xFF to the command, which will
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

#include "ws281x.hp"

#define GPIO1 0x4804c000
#define GPIO_CLEARDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194

#define WS821X_ENABLE (0x100)
#define DMX_CHANNELS (0x101)
#define DMX_PIN (0x102)

// Sleep a given number of nanoseconds with 10 ns resolution
// Uses r5
.macro SLEEPNS
.mparam ns,inst,lab
    //MOV r5, (ns/10)-1-inst
    MOV r5, (ns*10000)-1-inst
lab:
    SUB r5, r5, 1
    QBNE lab, r5, 0
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

#define SET_REG r6
#define CLR_REG r7

    // We will use these all the time
    MOV SET_REG, GPIO1 | GPIO_SETDATAOUT
    MOV CLR_REG, GPIO1 | GPIO_CLEARDATAOUT

    // Wait for the start condition from the main program to indicate
    // that we have a rendered frame ready to clock out.  This also
    // handles the exit case if an invalid value is written to the start
    // start position.
_LOOP:
    // Load the pointer to the buffer from PRU DRAM into r0 and the
    // length (in bytes-bit words) into r1.
    // start command into r2
    LBCO      r0, CONST_PRUDRAM, 0, 12

    // Wait for a non-zero command
    QBEQ _LOOP, r2, #0

    // Zero out the start command so that they know we have received it
    // This allows maximum speed frame drawing since they know that they
    // can now swap the frame buffer pointer and write a new start command.
    MOV r3, 0
    SBCO r3, CONST_PRUDRAM, 8, 4

    // Command of 0xFF is the signal to exit
    QBEQ EXIT, r2, #0xFF

    // Clock out the bits!
WORD_LOOP:
	// Load 32-bits of data into r3
	LBBO r3, r0, 0, 4

	// Start bit: set all bits on
	MOV r2, 0
	NOT r2, r2
	SBBO r2, SET_REG, 0, 4

	// wait for the length of the zero bits (250 ns)
	SLEEPNS 250, 1, wait_zero_time

	// For all the bits that are zero, turn them off now
	NOT r4, r3
	SBBO r4, CLR_REG, 0, 4
	
	// Wait until the length of the one bits (600 ns - 250 already waited)
	SLEEPNS 350, 1, wait_one_time

	// Turn all the bits off
	SBBO r2, CLR_REG, 0, 4

	// Wait until the length of the entire bit pulse (1.25 ns - 600 ns)
	SLEEPNS 625, 1, wait_end_time

	// Increment our data pointer and decrement our length
	ADD r0, r0, #4
	SUB r1, r1, #4
	QBGT WORD_LOOP, r1, #0

    // Delay at least 50 usec
    SLEEPNS 50000, 1, reset_time

    // Write out that we are done!
    // Store a non-zero response in the buffer so that they know that we are done
    // also write out a quick hack the last word we read
    MOV r2, #1
    SBCO r2, CONST_PRUDRAM, 12, 8

    // Go back to waiting for the next frame buffer
    QBA _LOOP

EXIT:
    // Write a 0xFF into the response field so that they know we're done
    MOV r2, #0xFF
    SBCO r2, CONST_PRUDRAM, 12, 4

#ifdef AM33XX
    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU0_ARM_INTERRUPT
#endif

    HALT
