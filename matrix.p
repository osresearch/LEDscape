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
#define gpio0_row1_r 2
#define gpio0_row1_g 3
#define gpio0_row1_b 7
#define gpio0_row2_r 5
#define gpio0_row2_b 4
#define gpio0_row2_g 12
#define gpio0_row3_r 13
#define gpio0_row3_g 14
#define gpio0_row3_b 15
#define gpio0_row4_r 20
#define gpio0_row4_g 22
#define gpio0_row4_b 23
#define gpio0_bit12 26
#define gpio0_bit13 27
#define gpio0_bit14 30
#define gpio0_bit15 31

// could move clock and sel into gpio0

// Pins available in GPIO1
#define gpio1_sel0 12
#define gpio1_sel1 13
#define gpio1_sel2 14
#define gpio1_latch 28
#define gpio1_oe 29
#define gpio1_clock 19

/** Generate a bitmask of which pins in GPIO0-3 are used.
 * 
 * \todo wtf "parameter too long": only 128 chars allowed?
 */
#define GPIO0_LED_MASK (0\
|(1<<gpio0_row1_r)\
|(1<<gpio0_row1_g)\
|(1<<gpio0_row1_b)\
|(1<<gpio0_row2_r)\
|(1<<gpio0_row2_b)\
|(1<<gpio0_row2_g)\
|(1<<gpio0_row3_r)\
|(1<<gpio0_row3_g)\
|(1<<gpio0_row3_b)\
|(1<<gpio0_row4_r)\
|(1<<gpio0_row4_g)\
|(1<<gpio0_row4_b)\
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


#define CLOCK(pin) \
	SLEEPNS 50, 1, pin##_on ; \
        SBBO clock_pin, gpio1_set, 0, 4; \
	SLEEPNS 50, 1, pin##_off ; \
        SBBO clock_pin, gpio1_clr, 0, 4; \

#define LATCH \
	SLEEPNS 50, 1, latch_on ; \
        SBBO latch_pin, gpio1_set, 0, 4; \
	SLEEPNS 50, 1, latch_off ; \
        SBBO latch_pin, gpio1_clr, 0, 4; \


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

#define DISPLAY_WIDTH          64
#define DISPLAYS               8 /* Maximum! */
#define ROW_WIDTH              (DISPLAYS * DISPLAY_WIDTH)

/*
for bright in 0..MAX_BRIGHT:
	for row in 0..7:
		set_address row
		offset = row * ROW_WIDTH
		for pixel in 0..32:
			for scan in 0..1:
				r_clr = r_set = 0
				g_clr = g_set = 0
				b_clr = b_set = 0
				for display in 0..DISPLAYS:
					read rgb from 4*(pixel + display*DISPLAY_WIDTH + offset)
					pin = display_pin(display)
					if r < bright:
						r_clr |= pin
					if b < bright:
						b_clr |= pin
					if g < bright:
						g_clr |= pin

				# All bitmasks have been built, clock them out
				clr_bits r_clr
				set_bits r_set
				clock
				clr_bits g_clr
				set_bits g_set
				clock
				clr_bits b_clr
				set_bits b_set
				clock

				# read the paired row for the next pass
				# on the second scan
				offset += 8 * DISPLAY_WIDTH

		# latch after both scan rows have been output
		latch
	# Check for new frame buffer now; this is a safe time to change it
*/

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
	//CLOCK
	//QBA PWM_LOOP

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
        
		// compute where we are in the image

		MOV pixel, 0
		ADD pix_ptr, data_addr, offset
		PIXEL_LOOP:
			MOV out0_set, 0

			// This should be unrolled for every display
			// read a pixel worth of data
			LBBO pix, pix_ptr, 0*DISPLAY_WIDTH, 4
			QBGE disp0_r, pix.b0, bright
			SET out0_set, gpio0_row1_r

			disp0_r:
			QBGE disp0_g, pix.b1, bright
			SET out0_set, gpio0_row1_g

			disp0_g:
			QBGE disp0_b, pix.b2, bright
			SET out0_set, gpio0_row1_b
			disp0_b:

			// All bits are configured;
			// the non-set ones will be cleared
			SBBO out0_set, gpio0_set, 0, 4
			XOR out0_set, out0_set, gpio0_led_mask
			SBBO out0_set, gpio0_clr, 0, 4
			CLOCK(PIX)

			ADD pix_ptr, pix_ptr, 4
			ADD pixel, pixel, 1
			QBNE PIXEL_LOOP, pixel, DISPLAY_WIDTH

                // We have clocked out all of the pixels for
                // this row and the one eigth rows later.
                // Latch the data
                LATCH

                ADD row, row, 1
		MOV p2, ROW_WIDTH * 4
                ADD offset, offset, p2
                QBNE ROW_LOOP, row, 8
    
        // We have clocked out all of the panels.
        // Celebrate and go back to the PWM loop
        // Limit brightness to 0..MAX_BRIGHT
        ADD bright, bright, 16
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
