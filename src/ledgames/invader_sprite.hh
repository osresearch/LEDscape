#pragma once

#include <iostream>
#include <cstdint>

#include "sprite.hh"

class invader_sprite_t : public sprite_t {
public:
  invader_sprite_t();
  
  virtual void draw_onto(Screen *screen) override;
  virtual void move_sprite(void) override;
  virtual void destroy_sprite(void);
  
private:
	uint32_t frames_in_anim;
};
