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
  virtual void set_image(uint8_t width, uint8_t height, uint32_t *pixels, uint8_t anim_frame = 0);
  virtual void set_image(uint16_t x_offset, uint16_t y_offset, uint8_t width, uint8_t height, png_t *png_image, uint8_t anim_frame = 0);
  virtual void draw_onto(Screen *screen);
  virtual void set_active(bool active);
  virtual bool is_active(void);
  virtual bool test_collision(const sprite_t &other_sprite);
  virtual float get_x_position(void);
  virtual float get_y_position(void);
  
  float x_;
  float y_;
  float dx_;
  float dy_;

  uint8_t width_;
  uint8_t height_;
  uint8_t frame_;

  bool active_;
  
  std::vector<uint32_t> sprite_data_[4];
};
