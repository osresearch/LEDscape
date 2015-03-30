#pragma once

#include <iostream>
#include <cstdint>

#include "sprite.hh"

class invader_sprite_t : public sprite_t {
public:
  invader_sprite_t(uint32_t score);
  
  virtual void draw_onto(Screen *screen) override;
  virtual void move_sprite(void) override;
  virtual uint32_t destroy_sprite(void);
  
private:
	uint32_t frames_in_anim_;
	uint32_t score_;
};
