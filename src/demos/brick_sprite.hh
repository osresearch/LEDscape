#pragma once

#include <iostream>
#include <cstdint>

#include "sprite.hh"

class brick_sprite_t : public sprite_t {
public:
  uint8_t score (void) { return score_; }
  uint8_t tone_number (void) { return tone_number_; }

  void set_score(uint8_t score) { score_ = score; }
  void set_tone_number(uint8_t tone_number) { tone_number_ = tone_number; }
  
private:
  uint8_t tone_number_;
  uint8_t score_;
  
};
