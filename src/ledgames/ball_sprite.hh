#pragma once

#include <iostream>
#include <cstdint>

#include "sprite.hh"

class ball_sprite_t : public sprite_t {
public:
  ball_sprite_t();
  
  virtual void move_sprite(void) override;
  void set_walls(bool up, bool down, bool left, bool right);
  
private:
  bool wall_left_;
  bool wall_right_;
  bool wall_up_;
  bool wall_down_;
};
