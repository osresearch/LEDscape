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
#define GPIO0_LED_MASK (0\
|(r11_gpio==0)<<r11_pin\
|(g11_gpio==0)<<g11_pin\
|(b11_gpio==0)<<b11_pin\
|(r12_gpio==0)<<r12_pin\
|(g12_gpio==0)<<g12_pin\
|(b12_gpio==0)<<b12_pin\
)

#define GPIO2_LED_MASK (0\
|(r11_gpio==2)<<r11_pin\
|(g11_gpio==2)<<g11_pin\
|(b11_gpio==2)<<b11_pin\
|(r12_gpio==2)<<r12_pin\
|(g12_gpio==2)<<g12_pin\
|(b12_gpio==2)<<b12_pin\
)


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
#define row r5
#define offset r6
#define scan r7
#define display_width_bytes r8
#define pixel r9
#define out_clr r10 // must be one less than out_set
#define out_set r11
#define p2 r12
#define bright r13
#define gpio0_led_mask r14
#define gpio2_led_mask r15
#define gpio1_sel_mask r16
#define pix r17
#define clock_pin r18
#define latch_pin r19
#define row11_ptr r20
#define row12_ptr r21
#define row21_ptr r22
#define row22_ptr r23
#define gpio0_set r24
#define gpio2_set r25

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

        MOV gpio1_sel_mask, GPIO1_SEL_MASK

        MOV gpio0_led_mask, 0
        MOV gpio2_led_mask, 0
#define GPIO_MASK(X) CAT3(gpio,X,_led_mask)
	SET GPIO_MASK(r11_gpio), r11_pin
	SET GPIO_MASK(g11_gpio), g11_pin
	SET GPIO_MASK(b11_gpio), b11_pin
	SET GPIO_MASK(r12_gpio), r12_pin
	SET GPIO_MASK(g12_gpio), g12_pin
	SET GPIO_MASK(b12_gpio), b12_pin

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
	MOV gpio2_set, 0

	LBBO pix, row11_ptr, offset, 4;
	QBGE skip_r11, pix.b0, bright;
	SET GPIO(r11_gpio), r11_pin;
	skip_r11:
	QBGE skip_g11, pix.b1, bright;
	SET GPIO(g11_gpio), g11_pin;
	skip_g11:
	QBGE skip_b11, pix.b2, bright;
	SET GPIO(b11_gpio), b11_pin;
	skip_b11:

	LBBO pix, row12_ptr, offset, 4;
	QBGE skip_r12, pix.b0, bright;
	SET GPIO(r12_gpio), r12_pin;
	skip_r12:
	QBGE skip_g12, pix.b1, bright;
	SET GPIO(g12_gpio), g12_pin;
	skip_g12:
	QBGE skip_b12, pix.b2, bright;
	SET GPIO(b12_gpio), b12_pin;
	skip_b12:

			// All bits are configured;
			// the non-set ones will be cleared
			// We write 8 bytes since CLR and DATA are contiguous,
			// which will write both the 0 and 1 bits in the
			// same instruction.
			AND out_set, gpio0_set, gpio0_led_mask
			XOR out_clr, out_set, gpio0_led_mask
			SBBO out_clr, gpio0_base, GPIO_CLRDATAOUT, 8

			AND out_set, gpio2_set, gpio2_led_mask
			XOR out_clr, out_set, gpio2_led_mask
			SBBO out_clr, gpio2_base, GPIO_CLRDATAOUT, 8

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
