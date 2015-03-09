#include "invader_sprite.hh"

invader_sprite_t::invader_sprite_t() :
  sprite_t()
{
}

void invader_sprite_t::move_sprite(void) {
  sprite_t::move_sprite();
  
  frame_ = (int)x_ % 2;
}
