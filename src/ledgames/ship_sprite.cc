#include "ship_sprite.hh"

ship_sprite_t::ship_sprite_t() :
sprite_t(),
frames_in_anim(0)
{
}

void ship_sprite_t::draw_onto(Screen *screen) {
	if (frame_ == 0) {
		frames_in_anim++;
		if (frames_in_anim > 3) {
			frames_in_anim = 0;
			frame_ = 1;
		}
	} else if (frame_ == 1) {
		frames_in_anim++;
		if (frames_in_anim > 3) {
			frames_in_anim = 0;
			frame_ = 0;
		}
	} else if (frame_ == 2) {
		frames_in_anim++;
		if (frames_in_anim > 3) {
			frames_in_anim = 0;
			frame_ = 3;
		}
	} else if (frame_ == 3) {
		frames_in_anim++;
		if (frames_in_anim > 3) {
			frames_in_anim = 0;
			explode_anims++;
			if (explode_anims > 10) {
				set_active(false);
				frame_ = 0;
			} else {
				frame_ = 2;
			}
		}
	}
	sprite_t::draw_onto(screen);
}

void ship_sprite_t::destroy_sprite(void) {
	frame_ = 2;
	frames_in_anim = 0;
	explode_anims = 0;
}