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
#define gpio1_sel0 12 /* 44, must be sequential with sel1 and sel2 */
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
#define GPIO0 (0x44E07000 + 0x100)
#define GPIO1 (0x4804c000 + 0x100)
#define GPIO2 (0x481AC000 + 0x100)
#define GPIO3 (0x481AE000 + 0x100)

/** Offsets for the clear and set registers in the devices.
 * Since the offsets can only be 0xFF, we deliberately add offsets
 */
#define GPIO_CLRDATAOUT (0x190 - 0x100)
#define GPIO_SETDATAOUT (0x194 - 0x100)

/** Register map */
#define data_addr r0
#define row_skip_bytes r1
#define gpio0_base r2
#define gpio1_base r3
#define row r4
#define offset r5
#define scan r6
#define display_width_bytes r7
#define pixel r8
#define out_clr r9 // must be one less than out_set
#define out_set r10
#define p2 r12
#define bright r13
#define gpio0_led_mask r14
#define gpio1_sel_mask r15
#define pix r16
#define clock_pin r17
#define latch_pin r18
#define row1_ptr r19
#define row2_ptr r20
#define row3_ptr r21
#define row4_ptr r22

#define BRIGHT_STEP 32

#define CLOCK_LO \
        SBBO clock_pin, gpio1_base, GPIO_SETDATAOUT, 4; \

#define CLOCK_HI \
        SBBO clock_pin, gpio1_base, GPIO_CLRDATAOUT, 4; \

#define LATCH_HI \
        SBBO latch_pin, gpio1_base, GPIO_SETDATAOUT, 4; \

#define LATCH_LO \
        SBBO latch_pin, gpio1_base, GPIO_CLRDATAOUT, 4; \

#define DISPLAY_OFF \
	MOV out_set, 0; \
	SET out_set, gpio1_oe; \
	SBBO out_set, gpio1_base, GPIO_SETDATAOUT, 4; \

#define DISPLAY_ON \
	MOV out_set, 0; \
	SET out_set, gpio1_oe; \
	SBBO out_set, gpio1_base, GPIO_CLRDATAOUT, 4; \


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

        MOV gpio0_base, GPIO0
        MOV gpio0_led_mask, GPIO0_LED_MASK

        MOV gpio1_base, GPIO1
        MOV gpio1_sel_mask, GPIO1_SEL_MASK

	MOV display_width_bytes, 4*DISPLAY_WIDTH
	MOV row_skip_bytes, 4*8*ROW_WIDTH

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

	// Store the pointers to each of the four outputs
	ADD row1_ptr, data_addr, 0
	ADD row2_ptr, row1_ptr, row_skip_bytes
	ADD row3_ptr, row1_ptr, display_width_bytes
	ADD row4_ptr, row3_ptr, row_skip_bytes

        ROW_LOOP:
		// compute where we are in the image
                MOV pixel, 0

		PIXEL_LOOP:
			MOV out_set, 0
			CLOCK_HI

			// read a pixel worth of data
			// \todo: track the four pointers separately
#define OUTPUT_ROW(N) \
	LBBO pix, row##N##_ptr, offset, 4; \
	QBGE skip_r##N, pix.b0, bright; \
	SET out_set, gpio0_r##N; \
	skip_r##N:; \
	QBGE skip_g##N, pix.b1, bright; \
	SET out_set, gpio0_g##N; \
	skip_g##N:; \
	QBGE skip_b##N, pix.b2, bright; \
	SET out_set, gpio0_b##N; \
	skip_b##N:; \

			OUTPUT_ROW(1)
			OUTPUT_ROW(2)
			OUTPUT_ROW(3)
			OUTPUT_ROW(4)

			// All bits are configured;
			// the non-set ones will be cleared
			// We write 8 bytes since CLR and DATA are contiguous,
			// which will write both the 0 and 1 bits in the
			// same instruction.
			XOR out_clr, out_set, gpio0_led_mask
			SBBO out_clr, gpio0_base, GPIO_CLRDATAOUT, 8

			CLOCK_LO

			// If the brightness is less than the pixel, turn off
			// but keep in mind that this is the brightness of
			// the previous row, not this one.
			// \todo: Test turning OE on and off every other,
			// every fourth, every eigth, etc pixel based on
			// the current brightness.
#if 1
			LSL p2, pixel, 2
			ADD p2, p2, pixel
			ADD p2, p2, pixel
			ADD p2, p2, pixel

			QBLT no_blank, bright, p2
			DISPLAY_OFF
			no_blank:
#endif

			ADD offset, offset, 4
			ADD pixel, pixel, 1
#if DISPLAY_WIDTH > 0xFF
			MOV p2, DISPLAY_WIDTH
			QBNE PIXEL_LOOP, pixel, p2
#else
			QBNE PIXEL_LOOP, pixel, DISPLAY_WIDTH
#endif

		// Disable output before we latch and set the address
		DISPLAY_OFF
		LATCH_HI

                // set address; select pins in gpio1 are sequential
		LSL out_set, row, gpio1_sel0
                XOR out_clr, out_set, gpio1_sel_mask
                SBBO out_clr, gpio1_base, GPIO_CLRDATAOUT, 8 // set both
        
                // We have clocked out all of the pixels for
                // this row and the one eigth rows later.
		// Latch the display register and then turn the display on
                LATCH_LO
		DISPLAY_ON

		// We have drawn half the image on each chain;
		// skip the second half
                ADD offset, offset, display_width_bytes

                ADD row, row, 1
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
