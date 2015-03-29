#pragma once

#include <iostream>
#include <cstdint>

#include "sprite.hh"

class ship_sprite_t : public sprite_t {
public:
  ship_sprite_t();
  
  virtual void draw_onto(Screen *screen) override;
  virtual void destroy_sprite(void);
  
private:
	uint32_t frames_in_anim;
	uint32_t explode_anims;
};
