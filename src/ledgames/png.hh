#pragma once

#include <iostream>
#include <cstdint>
#include <string>

#include <png.h>

class png_t {
public:
  png_t();
  ~png_t();

  virtual void read_file(std::string filename);
  virtual png_byte* row(uint16_t row_number);
  
private:
	int width_, height_;
	png_byte color_type_;
	png_byte bit_depth_;

	png_structp png_ptr_;
	png_infop info_ptr_;
	int number_of_passes_;
	png_bytep * row_pointers_;
	
};
