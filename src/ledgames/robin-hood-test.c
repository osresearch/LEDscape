/** \file
 * Test the matrix LCD PRU firmware with a multi-hue rainbow.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include "ledscape.h"


// Borrowed by OctoWS2811 rainbow test
static unsigned int
h2rgb(
	unsigned int v1,
	unsigned int v2,
	unsigned int hue
)
{
	if (hue < 60)
		return v1 * 60 + (v2 - v1) * hue;
	if (hue < 180)
		return v2 * 60;
	if (hue < 240)
		return v1 * 60 + (v2 - v1) * (240 - hue);

	return v1 * 60;
}


// Convert HSL (Hue, Saturation, Lightness) to RGB (Red, Green, Blue)
//
//   hue:        0 to 359 - position on the color wheel, 0=red, 60=orange,
//                            120=yellow, 180=green, 240=blue, 300=violet
//
//   saturation: 0 to 100 - how bright or dull the color, 100=full, 0=gray
//
//   lightness:  0 to 100 - how light the color is, 100=white, 50=color, 0=black
//
static uint32_t
makeColor(
	unsigned int hue,
	unsigned int saturation,
	unsigned int lightness
)
{
	unsigned int red, green, blue;
	unsigned int var1, var2;

	if (hue > 359)
		hue = hue % 360;
	if (saturation > 100)
		saturation = 100;
	if (lightness > 100)
		lightness = 100;

	// algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
	if (saturation == 0) {
		red = green = blue = lightness * 255 / 100;
	} else {
		if (lightness < 50) {
			var2 = lightness * (100 + saturation);
		} else {
			var2 = ((lightness + saturation) * 100) - (saturation * lightness);
		}
		var1 = lightness * 200 - var2;
		red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / 600000;
		green = h2rgb(var1, var2, hue) * 255 / 600000;
		blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / 600000;
	}
	return (red << 16) | (green << 8) | blue;
}



static uint32_t rainbowColors[180];


// phaseShift is the shift between each row.  phaseShift=0
// causes all rows to show the same colors moving together.
// phaseShift=180 causes each row to be the opposite colors
// as the previous.
//
// cycleTime is the number of milliseconds to shift through
// the entire 360 degrees of the color wheel:
// Red -> Orange -> Yellow -> Green -> Blue -> Violet -> Red
//
static void
rainbow(
	uint32_t * const pixels,
	unsigned width,
	unsigned height,
	unsigned phaseShift,
	unsigned cycle
)
{
	const unsigned color = cycle % 180;
	const unsigned dim = 128;

	static unsigned count = 0;

	count += 1;

	for (unsigned x=0; x < width; x++) {
		for (unsigned y=0; y < height; y++) {
			const int index = (color + x + y*phaseShift/4) % 180;
                        const uint32_t in  = rainbowColors[index];
			uint8_t * const out = &pixels[x + y*width];
#if 1
                        out[0] = ((in >> 0) & 0xFF) * dim / 128; // * y / 16;
                        out[1] = ((in >> 8) & 0xFF) * dim / 128; // * y / 16;
                        out[2] = ((in >> 16) & 0xFF) * dim / 128; // * y / 16;
#else
			if(y==((count >> 3) & 0x1F) && x<20) {
				out[0] = 0xff;
				out[1] = 0xff;
				out[2] = 0x00;
			}
			else {
				out[0] = 0x00;
				out[1] = 0x00;
				out[2] = 0xff;
			}
#endif
		}
	}
}


static void
gradient(
	uint32_t * const pixels,
	unsigned width,
	unsigned height,
	unsigned phaseShift,
	unsigned cycle
)
{
	cycle >>= 3;
	for (unsigned x=0; x < width; x++) {
		for (unsigned y=0; y < height; y++) {
			uint8_t * const out = &pixels[x + y*width];
#if 0
			//out[0] = ((x+cycle) % 32) * 8;
			//out[1] = ((y+cycle) % 16) * 16;
			uint8_t b = 0xFF;
			out[1] = b * ((((x + y + cycle) >> 5) ) & 1);
#else
			uint32_t b = 0;

			if (x % 32 < (x/32 + 4) && y % 32 < (y/32+1))
			{
				b = 0xFFFFFF;
			} else
			if (x < 32)
			{
				if (y < 32)
					b = 0xFF0000;
				else
				if (y < 64)
					b = 0x0000FF;
				else
				if (y < 96)
					b = 0x00FF00;
				else
					b = 0x411111;
			} else
			if (x < 64)
			{
				if (y < 32)
					b = 0xFF00FF;
				else
				if (y < 64)
					b = 0x00FFFF;
				else
				if (y < 96)
					b = 0xFFFF00;
				else
					b = 0x114111;
			} else {
				b = 0x111141;
			}
				
			out[0] = (b >> 16) & 0xFF;
			out[1] = (b >>  8) & 0xFF;
			out[2] = (b >>  0) & 0xFF;
			//*out = b;
#endif
		}
	}
}

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

void abort_(const char * s, ...)
{
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        abort();
}

int png_x, png_y;

int png_width, png_height;
png_byte color_type;
png_byte bit_depth;

png_structp png_ptr;
png_infop info_ptr;
int number_of_passes;
png_bytep * row_pointers;

void read_png_file(char* file_name)
{
        char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(file_name, "rb");
        if (!fp)
                abort_("[read_png_file] File %s could not be opened for reading", file_name);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
                abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
                abort_("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
                abort_("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        png_width = png_get_image_width(png_ptr, info_ptr);
        png_height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);


        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
                abort_("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * png_height);
        for (png_y=0; png_y<png_height; png_y++)
                row_pointers[png_y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
}

void process_file(uint32_t * const pixels)
{
        for (png_y=0; png_y<png_height; png_y++) {
                png_byte* row = row_pointers[png_y];
                for (png_x=0; png_x<png_width; png_x++) {
                        png_byte* ptr = &(row[png_x*4]);
                        uint8_t * const out = &pixels[png_x + png_y*png_width];
               		out[0] = ptr[2] >> 2;
			out[1] = ptr[1] >> 2;
			out[2] = ptr[0] >> 2; 
		}
        }
}

#define BUTTON_PLAYERSTART_P1 70
#define BUTTON_PLAYERSTART_P2 71
#define BUTTON_P1_JOYUP 79
#define BUTTON_P1_JOYDN 73
#define BUTTON_P1_JOYLEFT 74
#define BUTTON_P1_JOYRIGHT 75
#define BUTTON_P1_ACTION_PRI 76
#define BUTTON_P1_ACTION_SEC 77

void export_button_gpio (uint8_t button_number) {
	FILE *exportfile = fopen("/sys/class/gpio/export", "w");
	if (NULL == exportfile) {
		printf("Failed opening export device");
		return;
	}

	fprintf(exportfile, "%d", button_number);
	fclose(exportfile);
}

uint8_t button_pressed(uint8_t button_number) {
	char button_file_name[1024];
	snprintf(button_file_name, 1024, "/sys/class/gpio/gpio%d/value", button_number);
	FILE *button_file = fopen(button_file_name, "r");
	if (NULL == button_file) {
		printf("Failed opening button %d\n", button_number);
		return 0;
	}
	int button_value;
	fscanf(button_file, "%d", &button_value);
	fclose(button_file);
	return (button_value == 0) ? 1 : 0;
}

int
main(
	int argc,
	const char ** argv
)
{
	int width = 64;
	int height = 64;

	ledscape_config_t * config = &ledscape_matrix_default;
	if (argc > 1)
	{
		config = ledscape_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
	}

	if (config->type == LEDSCAPE_MATRIX)
	{
		config->matrix_config.width = width;
		config->matrix_config.height = height;
	}

	ledscape_t * const leds = ledscape_init(config, 0);

	export_button_gpio(BUTTON_PLAYERSTART_P1);
	export_button_gpio(BUTTON_PLAYERSTART_P2);
	export_button_gpio(BUTTON_P1_JOYUP);
        export_button_gpio(BUTTON_P1_JOYDN);
        export_button_gpio(BUTTON_P1_JOYLEFT);
        export_button_gpio(BUTTON_P1_JOYRIGHT);
        export_button_gpio(BUTTON_P1_ACTION_PRI);
        export_button_gpio(BUTTON_P1_ACTION_SEC);

	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	// pre-compute the 180 rainbow colors
	for (int i=0; i<180; i++)
	{
		int hue = i * 2;
		int saturation = 100;
		int lightness = 50;
		rainbowColors[i] = makeColor(hue, saturation, lightness);
	}

	read_png_file("/root/robin-hood.png");

	unsigned i = 0;
	uint32_t * const p = calloc(width*height,4);

	while (1)
	{
/*
		if (1)
			rainbow(p, width, height, 10, i++);
		else
			gradient(p, width, height, 10, i++);
*/
		process_file(p);
		i++;
		ledscape_draw(leds, p);

		if (button_pressed(BUTTON_PLAYERSTART_P1)) {
			printf("PLAYER 1 START\n");
		}
                if (button_pressed(BUTTON_PLAYERSTART_P2)) {
                        printf("PLAYER 2 START\n");
                }
                if (button_pressed(BUTTON_P1_JOYUP)) {
                        printf("P1-UP\n");
                }
                if (button_pressed(BUTTON_P1_JOYDN)) {
                        printf("P1-DOWN\n");
                }
                if (button_pressed(BUTTON_P1_JOYLEFT)) {
                        printf("P1-LEFT\n");
                }
                if (button_pressed(BUTTON_P1_JOYRIGHT)) {
                        printf("P1-RIGHT\n");
                }
                if (button_pressed(BUTTON_P1_ACTION_PRI)) {
                        printf("P1-PRI\n");
                }
                if (button_pressed(BUTTON_P1_ACTION_SEC)) {
                        printf("P1-SEC\n");
                }


		usleep(20000);

		// wait for the previous frame to finish;
		//const uint32_t response = ledscape_wait(leds);
		const uint32_t response = 0;
		time_t now = time(NULL);
		if (now != last_time)
		{
			printf("%d fps. starting %d previous %"PRIx32"\n",
				i - last_i, i, response);
			last_i = i;
			last_time = now;
		}

	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
