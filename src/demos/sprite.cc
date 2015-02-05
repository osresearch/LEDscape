#include "sprite.hh"

sprite_t::sprite_t() :
  x_(0),
  y_(0),
  dx_(0),
  dy_(0),
  active_(true)
{
}

sprite_t::~sprite_t() {
}

bool sprite_t::is_active(void) {
  return active_;
}

void sprite_t::set_active(bool active) {
  active_ = active;
}

void sprite_t::set_position(float x, float y) {
  x_ = x;
  y_ = y;
}

void sprite_t::set_speed(float dx, float dy) {
  dx_ = dx;
  dy_ = dy;
}

void sprite_t::move_sprite(void) {
  x_ += dx_;
  y_ += dy_;
}

void sprite_t::set_image(uint8_t width, uint8_t height, uint32_t *pixels) {
  sprite_data_.resize(width * height);
  sprite_data_.assign(pixels, pixels+(width * height));

  width_ = width;
  height_ = height;
}

void sprite_t::set_image(uint16_t x_offset, uint16_t y_offset, uint8_t width, uint8_t height, png_t *png_image) {
  sprite_data_.resize(width * height);

  width_ = width;
  height_ = height;

  for (uint8_t y = 0; y < height; y++) {
	  png_byte* row = png_image->row(y + y_offset);
	  for (uint8_t x = 0; x < width; x++) {
		  uint8_t *sprite_pixel = (uint8_t*)&(sprite_data_[x + (y * width)]);
		  uint8_t *png_pixel = (uint8_t*)&(row[(x + x_offset) * 4]);
		  sprite_pixel[0] = png_pixel[2];
		  sprite_pixel[1] = png_pixel[1];
		  sprite_pixel[2] = png_pixel[0];
	  }
  }
}

void sprite_t::draw_onto(Screen *screen) {
  if (!active_) return;
  
  for (uint8_t pixel_y = 0; pixel_y < height_; pixel_y++) {
    for (uint8_t pixel_x = 0; pixel_x < width_; pixel_x++) {
      screen->drawpixel(pixel_x + x_, pixel_y + y_, sprite_data_[(pixel_y * width_) + pixel_x]);
    }
  }
}

bool sprite_t::test_collision(const sprite_t &other_sprite) {
  if (!active_) return false;
  
  uint8_t visible_x = x_;
  uint8_t visible_y = y_;
  uint8_t other_visible_x = other_sprite.x_;
  uint8_t other_visible_y = other_sprite.y_;
  
  return !((visible_x + width_ - 1) < other_visible_x || (visible_y + height_ - 1) < (other_visible_y) ||
	   x_ > (other_visible_x + other_sprite.width_ - 1) ||
	   y_ > (other_visible_y + other_sprite.height_ - 1) );
}
