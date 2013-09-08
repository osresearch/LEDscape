// \file
 /* PRU based 16x32 LED Matrix driver.
 *
 * Drives up to sixteen 16x32 matrices using the PRU hardware.
 *
 * Uses sixteen data pins in GPIO0 (one for each data line on each
 * matrix) and six control pins in GPIO1 shared between all the matrices.
 *
 * The ARM writes a 24-bit color 512x16 into the shared RAM, sets the
 * frame buffer pointer in the command structure and the PRU clocks it out
 * to the sixteen matrices.  Since there is no PWM in the matrix hardware,
 * the PRU will cycle through various brightness levels. After each PWM
 * cycle it rechecks the frame buffer pointer, allowing a glitch-free
 * transition to a new frame.
 *
 * To pause the redraw loop, write a NULL to the buffer pointer.
 * To shut down the PRU, write -1 to the buffer pointer.
 *
 * HOW WE THINK THIS WORKS IS WRONG: it is not serial on all three rows,
 * but instead each R, G and B has its own input, and the two rows that
 * are simultaneously being scanned have their own.  So there are only 32
 * clocks for the input.
 *
 * This means that we need 6 IOs/panel for data, which is far more than
 * we thought.
 * 16 GPIO0 IOs == 2 with 4 IO left over.
 */
// Pins available in GPIO0
#define gpio0_r1 3
#define gpio0_g1 30
#define gpio0_b1 15

#define gpio0_r2 2
#define gpio0_b2 14
#define gpio0_g2 31

#define gpio0_r3 7
#define gpio0_b3 20
#define gpio0_g3 22

#define gpio0_r4 27
#define gpio0_b4 23
#define gpio0_g4 26

// Pins available in GPIO1
#define gpio1_sel0 12 /* 44 */
#define gpio1_sel1 13 /* 45 */
#define gpio1_sel2 14 /* 46 */
#define gpio1_latch 28 /* 60 */
#define gpio1_oe 15 /* 47 */
#define gpio1_clock 19 /* 51 */

/** Generate a bitmask of which pins in GPIO0-3 are used.
 * 
 * \todo wtf "parameter too long": only 128 chars allowed?
 */
#define GPIO0_LED_MASK (0\
|(1<<gpio0_r1)\
|(1<<gpio0_g1)\
|(1<<gpio0_b1)\
|(1<<gpio0_r2)\
|(1<<gpio0_g2)\
|(1<<gpio0_b2)\
|(1<<gpio0_r3)\
|(1<<gpio0_g3)\
|(1<<gpio0_b3)\
|(1<<gpio0_r4)\
|(1<<gpio0_g4)\
|(1<<gpio0_b4)\
)

#define GPIO1_SEL_MASK (0\
|(1<<gpio1_sel0)\
|(1<<gpio1_sel1)\
|(1<<gpio1_sel2)\
)

.origin 0
.entrypoint START

#include "ws281x.hp"

/** Mappings of the GPIO devices */
#define GPIO0 0x44E07000
#define GPIO1 0x4804c000
#define GPIO2 0x481AC000
#define GPIO3 0x481AE000

/** Offsets for the clear and set registers in the devices.
 * These are adjacent; can one sbbo instruction be used?
 */
#define GPIO_CLRDATAOUT 0x190
#define GPIO_SETDATAOUT 0x194

/** Register map */
#define data_addr r0
#define gpio0_set r20
#define gpio0_clr r21
#define gpio1_set r2
#define gpio1_clr r3
#define row r4
#define offset r5
#define scan r6
#define pix_ptr r7
#define pixel r8
#define out0_set r9
#define out1_set r10
#define p2 r12
#define bright r13
#define gpio0_led_mask r14
#define gpio1_sel_mask r15
#define pix r16
#define clock_pin r17
#define latch_pin r18

/** Sleep a given number of nanoseconds with 10 ns resolution.
 *
 * This busy waits for a given number of cycles.  Not for use
 * with things that must happen on a tight schedule.
 */
.macro SLEEPNS
.mparam ns,inst,lab
    MOV p2, (ns/10)-1-inst
lab:
    SUB p2, p2, 1
    QBNE lab, p2, 0
.endm


#define BRIGHT_STEP 32

#define CLOCK_LO \
        SBBO clock_pin, gpio1_set, 0, 4; \

#define CLOCK_HI \
        SBBO clock_pin, gpio1_clr, 0, 4; \

#define LATCH_HI \
        SBBO latch_pin, gpio1_set, 0, 4; \

#define LATCH_LO \
        SBBO latch_pin, gpio1_clr, 0, 4; \

#define DISPLAY_OFF \
	MOV out1_set, 0; \
	SET out1_set, gpio1_oe; \
	SBBO out1_set, gpio1_set, 0, 4; \

#define DISPLAY_ON \
	MOV out1_set, 0; \
	SET out1_set, gpio1_oe; \
	SBBO out1_set, gpio1_clr, 0, 4; \


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

    // Write a 0x1 into the response field so that they know we have started
    MOV r2, #0x1
    SBCO r2, CONST_PRUDRAM, 12, 4

    // Wait for the start condition from the main program to indicate
    // that we have a rendered frame ready to clock out.  This also
    // handles the exit case if an invalid value is written to the start
    // start position.

#define DISPLAY_WIDTH          128
#define DISPLAYS               2
#define ROW_WIDTH              (DISPLAYS * DISPLAY_WIDTH)

        MOV bright, #0

        MOV gpio0_set, GPIO0 | GPIO_SETDATAOUT
        MOV gpio0_clr, GPIO0 | GPIO_CLRDATAOUT
        MOV gpio0_led_mask, GPIO0_LED_MASK

        MOV gpio1_set, GPIO1 | GPIO_SETDATAOUT
        MOV gpio1_clr, GPIO1 | GPIO_CLRDATAOUT
        MOV gpio1_sel_mask, GPIO1_SEL_MASK

        MOV clock_pin, 0
        MOV latch_pin, 0
        SET clock_pin, gpio1_clock
        SET latch_pin, gpio1_latch

PWM_LOOP:
        // Load the pointer to the buffer from PRU DRAM into r0 and the
        // length (in bytes-bit words) into r1.
        // start command into r2
        LBCO      data_addr, CONST_PRUDRAM, 0, 4

        // Wait for a non-zero command
        QBEQ PWM_LOOP, data_addr, #0

        // Command of 0xFF is the signal to exit
        QBEQ EXIT, data_addr, #0xFF

        MOV offset, 0
        MOV row, 0

        ROW_LOOP:
		// compute where we are in the image

		MOV pixel, 0
		ADD pix_ptr, data_addr, offset

		PIXEL_LOOP:
			MOV out0_set, 0
			CLOCK_HI

			// read a pixel worth of data
#define OUTPUT_ROW(N, OFFSET) \
	MOV p2, (OFFSET); \
	LBBO pix, pix_ptr, p2, 4; \
	QBGE skip_r##N, pix.b0, bright; \
	SET out0_set, gpio0_r##N; \
	skip_r##N:; \
	QBGE skip_g##N, pix.b1, bright; \
	SET out0_set, gpio0_g##N; \
	skip_g##N:; \
	QBGE skip_b##N, pix.b2, bright; \
	SET out0_set, gpio0_b##N; \
	skip_b##N:; \

			OUTPUT_ROW(1, 0)
			OUTPUT_ROW(2, 8*ROW_WIDTH*4)
			OUTPUT_ROW(3, DISPLAY_WIDTH*4)
			OUTPUT_ROW(4, 8*ROW_WIDTH*4 + DISPLAY_WIDTH*4)

			// All bits are configured;
			// the non-set ones will be cleared
			SBBO out0_set, gpio0_set, 0, 4
			XOR out0_set, out0_set, gpio0_led_mask
			SBBO out0_set, gpio0_clr, 0, 4

			CLOCK_LO

			// If the brightness is less than the pixel, turn off
			// but keep in mind that this is the brightness of
			// the previous row, not this one.
#if 1
			LSL p2, pixel, 2
			ADD p2, p2, pixel
			ADD p2, p2, pixel
			ADD p2, p2, pixel

			SUB out0_set, bright, BRIGHT_STEP
			AND out0_set, out0_set, 0xFF

			QBLT no_blank, bright, p2
			DISPLAY_OFF
			no_blank:
#endif

			ADD pix_ptr, pix_ptr, 4
			ADD pixel, pixel, 1
			MOV p2, DISPLAY_WIDTH
			QBNE PIXEL_LOOP, pixel, p2

		// Disable output before we latch and set the address
		DISPLAY_OFF
		LATCH_HI

                // set address; pins in gpio1
                MOV out1_set, 0
                QBBC sel0, row, 0
                SET out1_set, gpio1_sel0
                sel0:
                QBBC sel1, row, 1
                SET out1_set, gpio1_sel1
                sel1:
                QBBC sel2, row, 2
                SET out1_set, gpio1_sel2
                sel2:

                // write select bits to output
                SBBO out1_set, gpio1_set, 0, 4
                XOR out1_set, out1_set, gpio1_sel_mask
                SBBO out1_set, gpio1_clr, 0, 4
        
                // We have clocked out all of the pixels for
                // this row and the one eigth rows later.
		// Latch the display register and then turn the display on
                LATCH_LO
		DISPLAY_ON

                ADD row, row, 1
		MOV p2, ROW_WIDTH * 4
                ADD offset, offset, p2
                QBNE ROW_LOOP, row, 8
    
        // We have clocked out all of the panels.
        // Celebrate and go back to the PWM loop
        // Limit brightness to 0..MAX_BRIGHT
        ADD bright, bright, BRIGHT_STEP
	AND bright, bright, 0xFF

        QBA PWM_LOOP
	
EXIT:
#ifdef AM33XX
    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU0_ARM_INTERRUPT
#endif

    HALT
