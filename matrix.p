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
#define r11_gpio 2
#define r11_pin 2
#define g11_gpio 2
#define g11_pin 3
#define b11_gpio 2
#define b11_pin 5

#define r12_gpio 0
#define r12_pin 23
#define g12_gpio 2
#define g12_pin 4
#define b12_gpio 0
#define b12_pin 26

#define r21_gpio 0
#define r21_pin 27
#define g21_gpio 2
#define g21_pin 1
#define b21_gpio 0
#define b21_pin 22

#define r22_gpio 2
#define r22_pin 22
#define g22_gpio 2
#define g22_pin 23
#define b22_gpio 2
#define b22_pin 24

#define r31_gpio 0
#define r31_pin 30
#define g31_gpio 1
#define g31_pin 18
#define b31_gpio 0
#define b31_pin 31

#define r32_gpio 1
#define r32_pin 16
#define g32_gpio 0
#define g32_pin 3
#define b32_gpio 0 // not working?
#define b32_pin 5

#define r41_gpio 0
#define r41_pin 2
#define g41_gpio 0
#define g41_pin 15
#define b41_gpio 1
#define b41_pin 17

#define r42_gpio 3
#define r42_pin 21
#define g42_gpio 3
#define g42_pin 19
#define b42_gpio 0
#define b42_pin 4

#define r51_gpio 2
#define r51_pin 25
#define g51_gpio 0
#define g51_pin 11
#define b51_gpio 0
#define b51_pin 10

#define r52_gpio 0
#define r52_pin 9
#define g52_gpio 0
#define g52_pin 8
#define b52_gpio 2
#define b52_pin 17

#define r61_gpio 2
#define r61_pin 16
#define g61_gpio 2
#define g61_pin 15
#define b61_gpio 2
#define b61_pin 14

#define r62_gpio 2
#define r62_pin 13
#define g62_gpio 2
#define g62_pin 10
#define b62_gpio 2
#define b62_pin 12

#define r71_gpio 2
#define r71_pin 11
#define g71_gpio 2
#define g71_pin 9
#define b71_gpio 2
#define b71_pin 8

#define r72_gpio 2
#define r72_pin 6
#define g72_gpio 0
#define g72_pin 7
#define b72_gpio 2
#define b72_pin 7

#define r81_gpio 3
#define r81_pin 17
#define g81_gpio 3
#define g81_pin 16
#define b81_gpio 3
#define b81_pin 15

#define r82_gpio 3
#define r82_pin 14
#define g82_gpio 0
#define g82_pin 14
#define b82_gpio 0
#define b82_pin 20

#define CAT3(X,Y,Z) X##Y##Z

// Control pins are all in GPIO1
#define gpio1_sel0 12 /* must be sequential with sel1 and sel2 */
#define gpio1_sel1 13
#define gpio1_sel2 14
#define gpio1_sel3 15
#define gpio1_latch 28
#define gpio1_oe 29
#define gpio1_clock 19

/** Generate a bitmask of which pins in GPIO0-3 are used.
 * 
 * \todo wtf "parameter too long": only 128 chars allowed?
 */

#define GPIO1_SEL_MASK (0\
|(1<<gpio1_sel0)\
|(1<<gpio1_sel1)\
|(1<<gpio1_sel2)\
|(1<<gpio1_sel3)\
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
#define gpio2_base r4
#define gpio3_base r9
#define row r5
#define offset r6
#define scan r7
#define display_width_bytes r8
#define out_clr r10 // must be one less than out_set
#define out_set r11
#define p2 r12
#define bright r13
#define gpio0_led_mask r14
#define gpio1_led_mask r27
#define gpio2_led_mask r15
#define gpio3_led_mask r28
#define gpio1_sel_mask r16
#define pixel r17
#define clock_pin r18
#define latch_pin r19
#define row11_ptr r20
#define row12_ptr r21
#define row21_ptr r22
#define row22_ptr r23
#define gpio0_set r24
#define gpio1_set r25
#define gpio2_set r26
#define gpio3_set r29

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
        MOV gpio1_base, GPIO1
        MOV gpio2_base, GPIO2
        MOV gpio3_base, GPIO3

        MOV gpio1_sel_mask, GPIO1_SEL_MASK

        MOV gpio0_led_mask, 0
        MOV gpio1_led_mask, 0
        MOV gpio2_led_mask, 0
        MOV gpio3_led_mask, 0

#define GPIO_MASK(X) CAT3(gpio,X,_led_mask)
	SET GPIO_MASK(r11_gpio), r11_pin
	SET GPIO_MASK(g11_gpio), g11_pin
	SET GPIO_MASK(b11_gpio), b11_pin
	SET GPIO_MASK(r12_gpio), r12_pin
	SET GPIO_MASK(g12_gpio), g12_pin
	SET GPIO_MASK(b12_gpio), b12_pin

	SET GPIO_MASK(r21_gpio), r21_pin
	SET GPIO_MASK(g21_gpio), g21_pin
	SET GPIO_MASK(b21_gpio), b21_pin
	SET GPIO_MASK(r22_gpio), r22_pin
	SET GPIO_MASK(g22_gpio), g22_pin
	SET GPIO_MASK(b22_gpio), b22_pin

	SET GPIO_MASK(r31_gpio), r31_pin
	SET GPIO_MASK(g31_gpio), g31_pin
	SET GPIO_MASK(b31_gpio), b31_pin
	SET GPIO_MASK(r32_gpio), r32_pin
	SET GPIO_MASK(g32_gpio), g32_pin
	SET GPIO_MASK(b32_gpio), b32_pin

	SET GPIO_MASK(r41_gpio), r41_pin
	SET GPIO_MASK(g41_gpio), g41_pin
	SET GPIO_MASK(b41_gpio), b41_pin
	SET GPIO_MASK(r42_gpio), r42_pin
	SET GPIO_MASK(g42_gpio), g42_pin
	SET GPIO_MASK(b42_gpio), b42_pin

	SET GPIO_MASK(r51_gpio), r51_pin
	SET GPIO_MASK(g51_gpio), g51_pin
	SET GPIO_MASK(b51_gpio), b51_pin
	SET GPIO_MASK(r52_gpio), r52_pin
	SET GPIO_MASK(g52_gpio), g52_pin
	SET GPIO_MASK(b52_gpio), b52_pin

	SET GPIO_MASK(r61_gpio), r61_pin
	SET GPIO_MASK(g61_gpio), g61_pin
	SET GPIO_MASK(b61_gpio), b61_pin
	SET GPIO_MASK(r62_gpio), r62_pin
	SET GPIO_MASK(g62_gpio), g62_pin
	SET GPIO_MASK(b62_gpio), b62_pin

	SET GPIO_MASK(r71_gpio), r71_pin
	SET GPIO_MASK(g71_gpio), g71_pin
	SET GPIO_MASK(b71_gpio), b71_pin
	SET GPIO_MASK(r72_gpio), r72_pin
	SET GPIO_MASK(g72_gpio), g72_pin
	SET GPIO_MASK(b72_gpio), b72_pin

	SET GPIO_MASK(r81_gpio), r81_pin
	SET GPIO_MASK(g81_gpio), g81_pin
	SET GPIO_MASK(b81_gpio), b81_pin
	SET GPIO_MASK(r82_gpio), r82_pin
	SET GPIO_MASK(g82_gpio), g82_pin
	SET GPIO_MASK(b82_gpio), b82_pin

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
	ADD row11_ptr, data_addr, 0
	ADD row12_ptr, row11_ptr, row_skip_bytes
	ADD row21_ptr, row11_ptr, display_width_bytes
	ADD row22_ptr, row21_ptr, row_skip_bytes

        ROW_LOOP:
		// compute where we are in the image
                MOV pixel, 0

		PIXEL_LOOP:
			MOV out_set, 0
			CLOCK_HI

#define GPIO(R) CAT3(gpio,R,_set)
	MOV gpio0_set, 0
	MOV gpio1_set, 0
	MOV gpio2_set, 0
	MOV gpio3_set, 0

#define OUTPUT_ROW(N) \
	LBBO p2, row##N##_ptr, offset, 4; \
	QBGE skip_r##N, p2.b0, bright; \
	SET GPIO(r##N##_gpio), r##N##_pin; \
	skip_r##N: \
	QBGE skip_g##N, p2.b1, bright; \
	SET GPIO(g##N##_gpio), g##N##_pin; \
	skip_g##N: \
	QBGE skip_b##N, p2.b2, bright; \
	SET GPIO(b##N##_gpio), b##N##_pin; \
	skip_b##N: \

#define OUTPUT_ROW2(P,N) \
	LBBO p2, row##P##_ptr, offset, 4; \
	QBGE skip_r##N, p2.b0, bright; \
	SET GPIO(r##N##_gpio), r##N##_pin; \
	skip_r##N: \
	QBGE skip_g##N, p2.b1, bright; \
	SET GPIO(g##N##_gpio), g##N##_pin; \
	skip_g##N: \
	QBGE skip_b##N, p2.b2, bright; \
	SET GPIO(b##N##_gpio), b##N##_pin; \
	skip_b##N: \

			OUTPUT_ROW(11)
			OUTPUT_ROW(12)
			OUTPUT_ROW(21)
			OUTPUT_ROW(22)
			OUTPUT_ROW2(11,81)
			OUTPUT_ROW2(12,82)

			// All bits are configured;
			// the non-set ones will be cleared
			// We write 8 bytes since CLR and DATA are contiguous,
			// which will write both the 0 and 1 bits in the
			// same instruction.
			AND out_set, gpio0_set, gpio0_led_mask
			XOR out_clr, out_set, gpio0_led_mask
			SBBO out_clr, gpio0_base, GPIO_CLRDATAOUT, 8

			AND out_set, gpio1_set, gpio1_led_mask
			XOR out_clr, out_set, gpio1_led_mask
			SBBO out_clr, gpio1_base, GPIO_CLRDATAOUT, 8

			AND out_set, gpio2_set, gpio2_led_mask
			XOR out_clr, out_set, gpio2_led_mask
			SBBO out_clr, gpio2_base, GPIO_CLRDATAOUT, 8

			AND out_set, gpio3_set, gpio3_led_mask
			XOR out_clr, out_set, gpio3_led_mask
			SBBO out_clr, gpio3_base, GPIO_CLRDATAOUT, 8

			CLOCK_LO

			// If the brightness is less than the pixel, turn off
			// but keep in mind that this is the brightness of
			// the previous row, not this one.
			// \todo: Test turning OE on and off every other,
			// every fourth, every eigth, etc pixel based on
			// the current brightness.
#if 1
			LSL p2, pixel, 1

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
		// Unless we've just done a full image, in which case
		// we treat this as a dummy row and go back to the top
		DISPLAY_OFF
                QBEQ NEXT_ROW, row, 8
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
		QBA ROW_LOOP
    
NEXT_ROW:
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
