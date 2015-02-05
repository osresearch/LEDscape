#include "ball_sprite.hh"

ball_sprite_t::ball_sprite_t() :
  sprite_t(),
  wall_left_(true),
  wall_right_(true),
  wall_up_(true),
  wall_down_(true)
{
}

void ball_sprite_t::move_sprite(void) {
  sprite_t::move_sprite();
  
  if (wall_left_) {
    if ((x_ < 0.0f) && (dx_ < 0)) {
      x_ = 0.0f;
      dx_ = -dx_;
    }
  }

  if (wall_right_) {
    if ((x_ > 63.0f) && (dx_ > 0)) {
      x_ = 63.0f;
      dx_ = -dx_;
    }
  }
  
  if (wall_up_) {
    if ((y_ < 0.0f) && (dy_ < 0)) {
      y_ = 0.0f;
      dy_ = -dy_;
    }
  }

  if (wall_down_) {
    if ((y_ > 63.0f) && (dy_ > 0)) {
      y_ = 63.0f;
      dy_ = -dy_;
    }
  }
}

void ball_sprite_t::set_walls(bool up, bool down, bool left, bool right) {
  wall_left_ = left;
  wall_right_ = right;
  wall_up_ = up;
  wall_down_ = down;
}
