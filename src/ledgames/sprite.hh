#pragma once

#include <iostream>
#include <cstdint>
#include <vector>

#include "screen.hh"
#include "png.hh"

class sprite_t {
public:
  sprite_t();
  ~sprite_t();

  virtual void set_position(float x, float y);
  virtual void set_speed(float dx, float dy);
  virtual void move_sprite(void);
  virtual void set_image(uint8_t width, uint8_t height, uint32_t *pixels);
  virtual void set_image(uint16_t x_offset, uint16_t y_offset, uint8_t width, uint8_t height, png_t *png_image);
  virtual void draw_onto(Screen *screen);
  virtual void set_active(bool active);
  virtual bool is_active(void);
  virtual bool test_collision(const sprite_t &other_sprite);

  float x_;
  float y_;
  float dx_;
  float dy_;

  uint8_t width_;
  uint8_t height_;

  bool active_;
  
  std::vector<uint32_t> sprite_data_;
};
