#include "invader_sprite.hh"

invader_sprite_t::invader_sprite_t(uint32_t score) :
sprite_t(),
frames_in_anim_(0),
score_(score)
{
}

void invader_sprite_t::draw_onto(Screen *screen) {
	if (frame_ == 2) {
		frames_in_anim_++;
		if (frames_in_anim_ > 3) {
			frames_in_anim_ = 0;
			frame_ = 3;
		}
	} else if (frame_ == 3) {
		frames_in_anim_++;
		if (frames_in_anim_ > 3) {
			frames_in_anim_ = 0;
			set_active(false);
		}
	}
	sprite_t::draw_onto(screen);
}

void invader_sprite_t::move_sprite(void) {
	sprite_t::move_sprite();
  
	if (frame_ < 2) {
		frame_ = (int)x_ & 0x01;
	}
}

uint32_t invader_sprite_t::destroy_sprite(void) {
	frame_ = 2;
	frames_in_anim_ = 0;
	return score_;
}