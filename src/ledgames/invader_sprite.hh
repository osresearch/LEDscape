#pragma once

#include <iostream>
#include <cstdint>

#include "sprite.hh"

class invader_sprite_t : public sprite_t {
public:
  invader_sprite_t();
  
  virtual void move_sprite(void) override;
  
private:
};
