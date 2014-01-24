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
#include <vector>

using namespace std;

#include "globals.h"
#include "pattern.h"
#include "pf2.h"
#include "ledscape.h"

static ledscape_t * leds;

// global levels to write to FPGA
uint16_t gLevels[DISPLAY_HEIGHT][DISPLAY_WIDTH];

// global object to create animated pattern
Perlin *gPattern = NULL;

// prototypes
void Quit (int sig);
void BlankDisplay (void);
void WriteLevels (void);
void timer_handler (int signum);

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

    config->matrix_config.width = DISPLAY_WIDTH;
    config->matrix_config.height = DISPLAY_HEIGHT;
    leds = ledscape_init(config);

    ledscape_printf((uint32_t*)(uintptr_t)gLevels, DISPLAY_WIDTH, 0xFF0000, "Perlin noise by Glen Akins");
    ledscape_draw(leds, gLevels);

    // initialize levels to all off
    BlankDisplay ();

    // create a new pattern object -- perlin noise, mode 2 long repeating
    gPattern = new Perlin (DISPLAY_WIDTH, DISPLAY_HEIGHT, 2, 6.0/64.0, 1.0/64.0, 256.0, 0.005);

    // create a new pattern object -- perlin noise, mode 1 short repeat
    // gPattern = new Perlin (DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, 8.0/64.0, 0.0125, 1.0, 0.2);

    // reset to first frame
    gPattern->init ();

    // install timer handler
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGALRM, &sa, NULL);

    // configure the timer to expire after 20 msec
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 20000;

    // and every 20 msec after that.
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 20000;

    // start the timer
    setitimer (ITIMER_REAL, &timer, NULL);

    // wait forever
    while (1) {
        sleep (1);
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


void timer_handler (int signum)
{
    // write levels to display
    WriteLevels ();

    // calculate next frame in animation
    if (gPattern != NULL) {
        bool patternComplete = gPattern->next ();
    }
}
