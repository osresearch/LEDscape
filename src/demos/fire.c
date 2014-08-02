/** \file
 * Draw fire patterns, derived from the pyramid Fire code.
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

// sideways pyramid; 256 height, but 128 wide
#define WIDTH 512
#define HEIGHT 64


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
hsv2rgb(
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
	return (red << 16) | (green << 8) | (blue << 0);
}


// This will contain the pixels used to calculate the fire effect
static uint8_t fire[WIDTH][HEIGHT];

  // Flame colors
static uint32_t palette[255];
static float angle;
static uint32_t calc1[WIDTH], calc2[HEIGHT], calc3[WIDTH], calc4[WIDTH], calc5[HEIGHT];

static void
fire_draw(
	uint32_t * const frame
)
{
    static int rotate_offset = 0;

    memset(frame, 0, WIDTH*HEIGHT*sizeof(*frame));

    angle = angle + 0.05;

    // Randomize the bottom row of the fire buffer
    for (int x = 0; x < WIDTH; x++)
    {
      fire[x][HEIGHT-1] = random() % 190;
    }

    int counter = 0;
    // Do the fire calculations for every pixel, from top to bottom
    for (int x = 0; x < WIDTH; x++) { // up to 128, leds_height
      for (int y = 0; y < HEIGHT; y++) { // up to 256, leds_width
        // Add pixel values around current pixel

        fire[x][y] =
          ((fire[calc3[x]][calc2[y]]
          + fire[calc1[x]][calc2[y]]
          + fire[calc4[x]][calc2[y]]
          + fire[calc1[x]][calc5[y]]) << 5) / (128+(abs(x-WIDTH/2))/4); // 129;

        // Output everything to screen using our palette colors
	const uint32_t c = palette[fire[x][y]];
        //frame[counter] = fire[x][y];

        // Extract the red value using right shift and bit mask 
        // equivalent of red(pgTemp.pixels[x+y*WIDTH])
	// Only map 3D cube 'lit' pixels onto fire array needed for next frame
        if (((c >> 0) & 0xFF) == 128)
          fire[x][y] = 128;

	// skip the bottom few rows
	/*
#if 1
	if (y > HEIGHT - leds_width)
		frame[y - (HEIGHT - leds_width) + x*leds_width] = c;
#else
	if (x > HEIGHT - leds_width)
		frame[y - (HEIGHT - leds_width) + x*leds_width] = c;
#endif
*/
	frame[WIDTH*y + (x + rotate_offset / 16) % WIDTH] = c;
	//frame[counter++] = c;
      }
    }

    rotate_offset++;
}


static void
sparkles(
	uint32_t * const frame,
	int num_sparkles
)
{
	for(int i = 0 ; i < num_sparkles ; i++)
		frame[rand() % (WIDTH*HEIGHT)] = 0xFFFFFF;
}

static int constrain(
	int x,
	int min,
	int max
)
{
	if (x < min)
		return min;
	if (x > max)
		return max;
	return x;
}

static void
init_pallete(void)
{
    for (int x = 0; x < 255; x++) {
      //Hue goes from 0 to 85: red to yellow
      //Saturation is always the maximum: 255
      //Lightness is 0..255 for x=0..128, and 255 for x=128..255
      palette[x] = hsv2rgb(x/2, 100, constrain(x, 0, 40));
    }

    // Precalculate which pixel values to add during animation loop
    // this speeds up the effect by 10fps
    for (int x = 0; x < WIDTH; x++) {
      calc1[x] = x % WIDTH;
      calc3[x] = (x - 1 + WIDTH) % WIDTH;
      calc4[x] = (x + 1) % WIDTH;
    }

    for (int y = 0; y < HEIGHT; y++) {
      calc2[y] = (y + 1) % HEIGHT;
      calc5[y] = (y + 2) % HEIGHT;
    }
}


int
main(
	int argc,
	const char ** argv
)
{
	ledscape_matrix_config_t * config = &ledscape_matrix_default;

	if (argc > 1)
	{
		config = ledscape_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
	}

	config->width = WIDTH;
	config->height = HEIGHT;
	ledscape_t * const leds = ledscape_init(config, 0);

	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	init_pallete();
	uint32_t * const p = calloc(WIDTH*HEIGHT,4);

	while (1)
	{
		// Alternate frame buffers on each draw command

		time_t now = time(NULL);
		const uint32_t delta = now - last_time;

		fire_draw(p);
		sparkles(p, delta);

		ledscape_draw(leds, p);
		usleep(50000);

		// wait for the previous frame to finish;
		//const uint32_t response = ledscape_wait(leds);
		const uint32_t response = 0;
		if (delta > 30)
		{
			printf("%d fps. starting %d previous %"PRIx32"\n",
				(i - last_i) / delta, i, response);
			last_i = i;
			last_time = now;
			memset(fire, 0, sizeof(fire));
		}

	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
