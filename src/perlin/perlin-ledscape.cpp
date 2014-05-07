//=============================================================================================
// LED Matrix Animated Pattern Generator
// Copyright 2014 by Glen Akins.
// Updated to work with LEDscape by Trammell Hudson
// All rights reserved.
// 
// Set editor width to 96 and tab stop to 4.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================================

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <memory.h>
#include <pthread.h>
#include <vector>

using namespace std;

#include "globals.h"
#include "pattern.h"
#include "pf2.h"
#include "ledscape.h"

static ledscape_t * leds;

// global levels to write to FPGA
uint32_t gLevels[DISPLAY_HEIGHT][DISPLAY_WIDTH];

// global object to create animated pattern
Perlin *gPattern = NULL;

// prototypes
void Quit (int sig);
void BlankDisplay (void);
void WriteLevels (void);
void timer_handler (int signum);


static void *
read_thread(
	void * arg
)
{
	char line[128];
	float new_size;
	printf("read thread waiting\n");
	int old_p0 = -1;
	int old_p1 = -1;
	int old_p2 = -1;
	int old_p3 = -1;

	while (1)
	{
		if (fgets(line, sizeof(line), stdin) == NULL)
			break;
		//printf("read '%s'\n", line);
		int p0, p1, p2, p3;
		int rc = sscanf(line, "%d %d %d %d", &p0, p1, p2, p3);
		if (rc != 4)
		{
			printf("scanf failed: %d: '%s'\n", rc, line);
			continue;
		}

		if (p0 != old_p0)
			gPattern->setScale(p0 / 1024.0);
		old_p0 = p0;
	}

	fprintf(stderr, "read thread exiting\n");
	return NULL;
}

int main (int argc, char *argv[])
{
    struct sigaction sa;
    struct itimerval timer;

    // trap ctrl-c to call quit function 
    signal (SIGINT, Quit);

    ledscape_config_t * config = &ledscape_matrix_default;
    if (argc > 1)
    {
	config = ledscape_config(argv[1]);
	if (!config)
		return EXIT_FAILURE;
    }

    float hue_options = 0.005;
    if (argc > 2)
	hue_options = atof(argv[2]);

    config->matrix_config.width = DISPLAY_WIDTH;
    config->matrix_config.height = DISPLAY_HEIGHT;
    leds = ledscape_init(config, 0);

    ledscape_printf((uint32_t*)(uintptr_t)gLevels, DISPLAY_WIDTH, 0xFF0000, "Perlin noise by Glen Akins");
    ledscape_draw(leds, gLevels);

    // initialize levels to all off
    BlankDisplay ();

    // create a new pattern object -- perlin noise, mode 2 long repeating
    gPattern = new Perlin(
	DISPLAY_WIDTH,
	DISPLAY_HEIGHT,
	2, // mode
	3.0/64.0, // size of blobs: smaller value == larger blob
	1.0/64.0, // speed
	256.0,
	hue_options
    );

    // spin off a thread to read size, speed and other settings
    pthread_t read_id;
    if (pthread_create(&read_id, NULL, read_thread, NULL) < 0)
	    return EXIT_FAILURE;

    // create a new pattern object -- perlin noise, mode 1 short repeat
    // gPattern = new Perlin (DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, 8.0/64.0, 0.0125, 1.0, 0.2);

    // reset to first frame
    gPattern->init ();

	while (1) {
		WriteLevels ();

		// calculate next frame in animation
		gPattern->next ();
		usleep(1000);
	}

    // delete pattern object
    delete gPattern;

    return 0;
}


void Quit (int sig)
{
    exit (-1);
}


void BlankDisplay (void)
{
    // initialize levels to all off
    for (int32_t row = 0; row < DISPLAY_HEIGHT; row++) {
        for (int32_t col = 0; col < DISPLAY_WIDTH; col++) {
            gLevels[row][col] = 0x0000;
        }
    }

    // send levels to board
    WriteLevels ();
}


void WriteLevels (void)
{
    ledscape_draw(leds, gLevels);
}

