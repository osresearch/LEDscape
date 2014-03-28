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

static const int w = 256;
static const int h = 36;
static const int leds_height = 128;
static const int leds_width = 128;


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
	return (blue << 16) | (green << 8) | red;
}


// This will contain the pixels used to calculate the fire effect
static uint8_t fire[256][64];

  // Flame colors
static uint32_t palette[255];
static float angle;
static uint32_t calc1[256], calc2[256], calc3[256], calc4[256], calc5[256];

static void
fire_draw(
	uint32_t * const frame
)
{
    memset(frame, 0, w*h*sizeof(*frame));

    angle = angle + 0.05;

    // Randomize the bottom row of the fire buffer
    for (int x = 0; x < w; x++)
    {
      fire[x][h-1] = random() % 190;
    }

    int counter = 0;
    // Do the fire calculations for every pixel, from top to bottom
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        // Add pixel values around current pixel

        fire[x][y] =
          ((fire[calc3[x]][calc2[y]]
          + fire[calc1[x]][calc2[y]]
          + fire[calc4[x]][calc2[y]]
          + fire[calc1[x]][calc5[y]]) << 5) / (128+(abs(x-w/2))/4); // 129;

        // Output everything to screen using our palette colors
	const uint32_t c = palette[fire[x][y]];
        //frame[counter] = fire[x][y];

        // Extract the red value using right shift and bit mask 
        // equivalent of red(pgTemp.pixels[x+y*w])
	// Only map 3D cube 'lit' pixels onto fire array needed for next frame
        if (((c >> 0) & 0xFF) == 128)
          fire[x][y] = 128;

	frame[counter++] = c;
      }
    }
}


static void
sparkles(
	uint32_t * const frame,
	int num_sparkles
)
{
	for(int i = 0 ; i < num_sparkles ; i++)
		frame[rand() % (16*256)] = 0xFFFFFF;
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
    for (int x = 0; x < w; x++) {
      calc1[x] = x % w;
      calc3[x] = (x - 1 + w) % w;
      calc4[x] = (x + 1) % w;
    }

    for (int y = 0; y < h; y++) {
      calc2[y] = (y + 1) % h;
      calc5[y] = (y + 2) % h;
    }
}


int
main(void)
{
	ledscape_t * const leds = ledscape_init(leds_width,leds_height);
	printf("init done\n");
	time_t last_time = time(NULL);
	unsigned last_i = 0;

	unsigned i = 0;
	init_pallete();
	uint32_t * const p = calloc(w*h,4);
	uint32_t * const fb = calloc(leds_width*leds_height,4);

	while (1)
	{
		// Alternate frame buffers on each draw command

		time_t now = time(NULL);
		const uint32_t delta = now - last_time;

		fire_draw(p);
		sparkles(p, delta);
		framebuffer_flip(fb, p, leds_width, leds_height, w, h);
		ledscape_draw(leds, fb);
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
