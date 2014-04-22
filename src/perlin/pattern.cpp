//=============================================================================================
// LED Matrix Animated Pattern Generator
// Copyright 2014 by Glen Akins.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "globals.h"
#include "gammalut.h"
#include "pattern.h"

#define MAKE_COLOR(r,g,b) \
	(((r) << 16) | ((g) << 8) | ((b) << 0))

//---------------------------------------------------------------------------------------------
// convert a hue from 0 to 95 to its 12-bit RGB color
//
// hue: 0 = red, 32 = blue, 64 = green
//

uint32_t Pattern::translateHue (int32_t hue)
{
    uint8_t hi, lo;
    uint32_t r, g, b;

    hi = hue >> 4;
    lo = ((hue & 0xf) << 4) | (hue & 0xf);

#if 1
    switch (hi) {
        case 0: r = 0xff;    g = 0;       b = lo;      break;
        case 1: r = 0xff-lo, g = 0,       b = 0xff;    break;
        case 2: r = 0,       g = lo,      b = 0xff;    break;
        case 3: r = 0,       g = 0xff,    b = 0xff-lo; break;
        case 4: r = lo,      g = 0xff,    b = 0;       break;
        case 5: r = 0xff,    g = 0xff-lo, b = 0;       break;
    }
//printf("hue=%d=%d,%d: %d,%d,%d\n", hue, hi, lo, r, g, b);
    //r = gammaLut[r] << 3;
    //g = gammaLut[g] << 3;
    //b = gammaLut[b] << 3;
    r /= 4;
    b /= 4;
    g /= 4;
#else
	b = hue;
	r = 0;
	g = 0;
#endif


    uint32_t color = MAKE_COLOR (r,g,b);
	//printf("color=%08x\n", color);
return color;
}


//---------------------------------------------------------------------------------------------
// convert a hue from 0 to 95 and a brightnewss from 0 to 1.0 to its 12-bit RGB color
//
// hue: 0 = red, 32 = blue, 64 = green
// value: 0 = off, 1.0 = 100%
//

uint32_t Pattern::translateHueValue (int32_t hue, float value)
{
    uint8_t hi, lo;
    uint8_t r, g, b;

    hi = hue >> 4;
    lo = ((hue & 0xf) << 4) | (hue & 0xf);

    switch (hi) {
        case 0: r = 0xff;    g = 0;       b = lo;      break;
        case 1: r = 0xff-lo, g = 0,       b = 0xff;    break;
        case 2: r = 0,       g = lo,      b = 0xff;    break;
        case 3: r = 0,       g = 0xff,    b = 0xff-lo; break;
        case 4: r = lo,      g = 0xff,    b = 0;       break;
        case 5: r = 0xff,    g = 0xff-lo, b = 0;       break;
    }

    r = ((float)r + 0.5) * value;
    g = ((float)g + 0.5) * value;
    b = ((float)b + 0.5) * value;

    r = gammaLut[r] << 4;
    g = gammaLut[g] << 4;
    b = gammaLut[b] << 4;

    return MAKE_COLOR (r,g,b);
}
