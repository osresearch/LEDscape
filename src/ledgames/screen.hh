#pragma once
#include <iostream>
#include <cstdint>

#include "ledscape.h"

class Screen {
public:
  Screen(ledscape_t * const leds, uint32_t *pixels);
  ~Screen();

  void drawpixel(uint32_t x, uint32_t y, uint32_t color);
  void set_background_color(uint32_t color);
  void draw_start(void);
  void draw_end(void);
  void draw_text(uint32_t row, uint32_t column, uint32_t color, std::string output, bool flip = false);
  void set_flip(bool do_flip);
  
private:
  void draw_char(uint32_t row, uint32_t column, const uint32_t color, char c, bool flip);

  uint32_t *pixels_;
  ledscape_t * const leds_;
  bool flip_;

  uint32_t background_color_;
};
