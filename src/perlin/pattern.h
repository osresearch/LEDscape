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

#ifndef __pattern_h_
#define __pattern_h_

extern const uint8_t gammaLut[];

class Pattern
{
    public:

        // constructor
        Pattern (const int32_t width, const int32_t height) :
            m_width(width), m_height(height) { }

        // destructor
        ~Pattern (void) { }

        // reset to first frame in animation
        virtual void init (void) = 0;

        // calculate next frame in the animation
        virtual bool next (void) = 0;

        // get width and height
        void getDimensions (int32_t &width, int32_t &height) {
            width = m_width; height = m_height;
        }

        uint16_t translateHue (int32_t hue);
        uint16_t translateHueValue (int32_t hue, float value);
        
    protected:
        const int32_t m_width;
        const int32_t m_height;

    private:
};

#endif
