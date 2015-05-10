#include <unistd.h>
#include "screen.hh"

extern const uint8_t fixed_font[][5];

Screen::Screen(ledscape_t * const leds, uint32_t *pixels):
pixels_(pixels),
leds_(leds),
flip_(false)
{
	set_background_color(0x00000000);
	draw_start();
	ledscape_printf(&pixels_[9], 64, 0x00401010, "LedGames\n Engine");
	ledscape_printf(&pixels_[64*32 + 17], 64, 0x00101040, "Keith");
	ledscape_printf(&pixels_[64*40 + 2], 64, 0x00101040, "Henrickson");
	draw_end();
	sleep(1);
}

Screen::~Screen()
{
	set_background_color(0x00000000);
	draw_start();
	draw_end();
	sleep(1);
}

void Screen::drawpixel(uint32_t x, uint32_t y, uint32_t color) {
	if ((x > 63) || (y > 63)) {
		return;
	}
	uint32_t pixelnum = ((flip_ ? (63 - y) : y) * 64) + (flip_ ? (63 - x ) : x);
	pixels_[pixelnum] = color;
}

void Screen::set_background_color(uint32_t color) {
	background_color_ = color;
}

void Screen::draw_char(uint32_t row, uint32_t column, const uint32_t color, char c, bool flip) {
	if (c < 0x20 || c > 127)
		c = '?';
  
	const uint8_t* const f = fixed_font[c - 0x20];
	for (int x = 0 ; x < 5 ; x++) {
		uint8_t bits = f[x];
		for (int y = 0 ; y < 7 ; y++, bits >>= 1) {
			if (flip) {
				drawpixel((4-x) + column, (6-y) + row, (bits & 1) ? color : 0);
			} else {
				drawpixel(x + column, y + row, (bits & 1) ? color : 0);
			}
		}
	}
}

void Screen::draw_text(uint32_t row, uint32_t column, uint32_t color, std::string output, bool flip) {
	if (flip) {
		column += (output.length() - 1) * 6;
	}
	for (unsigned i = 0 ; i < output.length() ; i++) {
		char c = output[i];
		if (!c)
			break;
    
		draw_char(row, column, color, c, flip);
		if (flip) {
			column -= 6;
		} else {
			column += 6;
		}
	}
}

void Screen::draw_start(void) {
	for (int counter = 0; counter < (64*64); counter++) {
		pixels_[counter] = background_color_;
	}
}

void Screen::draw_end(void) {
	ledscape_draw(leds_, pixels_);
}

void Screen::set_flip(bool do_flip) {
	flip_ = do_flip;
}
