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
//
// Major inspiration for the use of Perlin noise to generate pseudorandom RGB patterns comes
// from the TI RGB LED coffee table project and the following resources:
//
// TI RGB LED Coffee Table:
//
//  http://e2e.ti.com/group/microcontrollerprojects/m/msp430microcontrollerprojects/447779.aspx
//  https://github.com/bear24rw/rgb_table/tree/master/code/table_drivers/pytable
//
// Casey Duncan's Python C Noise Library:
//
//  https://github.com/caseman/noise
//
// Ken Perlin's Original Source Code:
//
//  http://www.mrl.nyu.edu/~perlin/doc/oscar.html
//
// Excellent explanation of Perlin noise and seamless looping and tiling here: 
// 
//  http://webstaff.itn.liu.se/~stegu/TNM022-2005/perlinnoiselinks/perlin-noise-math-faq.html
//
//=============================================================================================

#ifndef __pf2_h_
#define __pf2_h_

class Perlin : public Pattern
{
    public:
        
        // constructor
        Perlin (const int32_t width, const int32_t height, int32_t mode);

        // constructor
        // m_hue_option is hue offset from 0.0 to 1.0 for mode 1, hue step for modes 2 and 3
        Perlin (const int32_t width, const int32_t height,
            const int32_t mode, const float xy_scale, 
            const float z_step, const float z_depth, const float hue_options);

        // destructor
        ~Perlin (void);

        // reset to first frame in animation
        void init (void);

        // calculate next frame in the animation
        bool next (void);

        // get / set scale
        float getScale (void) {
            return m_xy_scale / 256.0;
        }
        void setScale (float xy_scale) {
            m_xy_scale = xy_scale * 256.0;
        }

        // get / set z step
        float getZStep (void) {
            return m_z_step;
        }
        void setZStep (float z_step) {
            m_z_step = z_step;
        }

        // get / set z depth
        float getZDepth (void) {
            return m_z_depth;
        }
        void setZDepth (float z_depth) {
            m_z_depth = z_depth;
            m_z_state = 0;                  
        }

        // get / set hue options
        float getHueOptions (void) {
            return m_hue_options;
        }
        void setHueOptions (float hue_options) {
            m_hue_options = hue_options;
        }

    private:

        // 3d perlin noise function
		int32_t noise (uint16_t x, uint16_t y, uint16_t z);

        // mode:
        //   1 = fixed background hue
        //   2 = hue rotates and varies with noise
        //   3 = hue rotates, noise varies brightness
        const int32_t m_mode; 

        // x and y scale of noise
        int32_t m_xy_scale;

        // step in the z direction between displayed x-y planes
        float m_z_step;
    
        // depth in the z direction before the pattern repeats
        float m_z_depth;

        // background hue for mode 1, from 0.0 to 1.0
        // hue step size for modes 2 and 3
        float m_hue_options;

        // current z coordinate, mod z depth
        float m_z_state;
    
        // current hue, mod 1.0
        float m_hue_state;
        
        // current minimum and maximum noise values for normalization
        float m_min, m_max;
};

#endif
