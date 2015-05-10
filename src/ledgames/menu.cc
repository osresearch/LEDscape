/** \file
* Test the matrix LCD PRU firmware with a multi-hue rainbow.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#include <random>
#include <unistd.h>

#include <vector>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "ledscape.h"

#include "controls.hh"
#include "screen.hh"
#include "sprite.hh"
#include "png.hh"

static controls_t *player_controls[2];

static Mix_Chunk *startup_bong;
static Mix_Chunk *menu_blip;

static png_t sprite_sheet;
static std::vector<sprite_t> game_sprites;

static uint32_t current_option;

static uint32_t marque_position;
static uint32_t text_color;

enum class game_state_t {
	Menu,
	Menu_Select,
	Service,
};
  
static game_state_t game_state;

static uint32_t rainbowColors[180];

static char marque_text[] = "Welcome to LED Games at MakerFaire Bay Area 2015!";

// Borrowed by OctoWS2811 rainbow test
static unsigned int
h2rgb(
	unsigned int v1,
	unsigned int v2,
	unsigned int hue
)
{
	if (hue < 60)
		return v1 * 60 + (v2 - v1) * hue;
	if (hue < 180)
		return v2 * 60;
	if (hue < 240)
		return v1 * 60 + (v2 - v1) * (240 - hue);

	return v1 * 60;
}

// Convert HSL (Hue, Saturation, Lightness) to RGB (Red, Green, Blue)
//
//   hue:        0 to 359 - position on the color wheel, 0=red, 60=orange,
//                            120=yellow, 180=green, 240=blue, 300=violet
//
//   saturation: 0 to 100 - how bright or dull the color, 100=full, 0=gray
//
//   lightness:  0 to 100 - how light the color is, 100=white, 50=color, 0=black
//
static uint32_t
makeColor(
	unsigned int hue,
	unsigned int saturation,
	unsigned int lightness
)
{
	unsigned int red, green, blue;
	unsigned int var1, var2;

	if (hue > 359)
		hue = hue % 360;
	if (saturation > 100)
		saturation = 100;
	if (lightness > 100)
		lightness = 100;

	// algorithm from: http://www.easyrgb.com/index.php?X=MATH&H=19#text19
	if (saturation == 0) {
		red = green = blue = lightness * 255 / 100;
	} else {
		if (lightness < 50) {
			var2 = lightness * (100 + saturation);
		} else {
			var2 = ((lightness + saturation) * 100) - (saturation * lightness);
		}
		var1 = lightness * 200 - var2;
		red = h2rgb(var1, var2, (hue < 240) ? hue + 120 : hue - 240) * 255 / 600000;
		green = h2rgb(var1, var2, hue) * 255 / 600000;
		blue = h2rgb(var1, var2, (hue >= 120) ? hue - 120 : hue + 240) * 255 / 600000;
	}
	return (red << 16) | (green << 8) | blue;
}

static void reset_menu(void) {
	game_state = game_state_t::Menu;

	sprite_t bricks_sprite;
	bricks_sprite.set_active(true);
	bricks_sprite.set_position(0,0);
	bricks_sprite.set_image(0,0,64,32,&sprite_sheet);
	
	sprite_t paddles_sprite;
	paddles_sprite.set_active(false);
	paddles_sprite.set_position(0,0);
	paddles_sprite.set_image(64,0,64,32,&sprite_sheet);
	
	sprite_t invaders_sprite;
	invaders_sprite.set_active(false);
	invaders_sprite.set_position(0,0);
	invaders_sprite.set_image(128,0,64,32,&sprite_sheet);
	
	game_sprites.clear();
	game_sprites.push_back(bricks_sprite);
	game_sprites.push_back(paddles_sprite);
	game_sprites.push_back(invaders_sprite);
	
	current_option = 0;
	
	marque_position = 64;

	// pre-compute the 180 rainbow colors
	for (int i=0; i<180; i++)
	{
		int hue = i * 2;
		int saturation = 100;
		int lightness = 50;
		rainbowColors[i] = makeColor(hue, saturation, lightness);
	}
	text_color = 0;

	printf("\n\n\nGAME START\n\n\n");
}

void render_game(Screen *screen) {
	screen->set_background_color(0x00000000);
	screen->draw_start();

	player_controls[0]->refresh_status();
	player_controls[1]->refresh_status();
  
	if ((game_state == game_state_t::Menu) || (game_state == game_state_t::Menu_Select)) {
		screen->draw_text(32,marque_position,rainbowColors[text_color % 180],marque_text);
		screen->draw_text(40,2,rainbowColors[text_color % 180],"Joy. sel.");
		screen->draw_text(48,2,rainbowColors[text_color % 180],"Btn. start");

		for (auto sprite : game_sprites) {
			sprite.draw_onto(screen);
		}
		
		marque_position--;
		if (marque_position == (0xFFFFFFFF - (5 * sizeof(marque_text)) - 64)) {
			marque_position = 64;
		}
		
		text_color++;
	}
  
	screen->draw_end();
  
	if (game_state == game_state_t::Menu) {
		if (player_controls[0]->is_pressed(button_a)) {
			exit(current_option);
		}
	
		if (player_controls[0]->is_pressed(joystick_left)) {
			game_sprites[current_option].set_active(false);
			current_option = (current_option > 0) ? current_option-1 : 0;
			game_sprites[current_option].set_active(true);
			game_state = game_state_t::Menu_Select;
		}
   
		if (player_controls[0]->is_pressed(joystick_right)) {
			game_sprites[current_option].set_active(false);
			current_option = (current_option < game_sprites.size()-1) ? current_option+1 : game_sprites.size()-1;
			game_sprites[current_option].set_active(true);
			game_state = game_state_t::Menu_Select;
		}
	}

	if (game_state == game_state_t::Menu_Select) {
		if ((!player_controls[0]->is_pressed(joystick_left)) && (!player_controls[0]->is_pressed(joystick_right))) {
			game_state = game_state_t::Menu;
		}
	}
}

static void init_sdl(void) {
	if (SDL_Init(SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
		exit(1);
	}

	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16SYS;
	int audio_channels = 2;
	int audio_buffers = 16384;

	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers) != 0) {
		fprintf(stderr, "Unable to initialize audio: %s\n", Mix_GetError());
		exit(1);
	}

	startup_bong = Mix_LoadWAV("bin/startup.wav");
	if (startup_bong == NULL) {
		fprintf(stderr, "Unable to load startup.wav: %s\n", Mix_GetError());
		exit(1);
	}

	Mix_PlayChannel(-1, startup_bong, 0);
}

int
	main(
		int argc,
const char ** argv
	)
{
	int width = 64;
	int height = 64;

	ledscape_config_t * config = &ledscape_matrix_default;
	if (argc > 1)
	{
		config = ledscape_config(argv[1]);
		if (!config)
			return EXIT_FAILURE;
	}

	if (config->type == LEDSCAPE_MATRIX)
	{
		config->matrix_config.width = width;
		config->matrix_config.height = height;
	}

	sprite_sheet.read_file("bin/Menus.png");

	ledscape_t * const leds = ledscape_init(config, 0);

	player_controls[0] = new controls_t(1);
	player_controls[1] = new controls_t(3);
  
	reset_menu();
  
	printf("init done\n");
	uint32_t * const p = (uint32_t*)calloc(width*height,4);

	Screen *screen = new Screen(leds, p);
  
	init_sdl();

	menu_blip = Mix_LoadWAV("bin/blip3.wav");
  
	while (1)
	{
		try {
			render_game(screen);
			usleep(20000);
		}
		catch (control_exit_exception* ex) {
			// Ignore the exception, as there is no useful exit program.
		}
	}

	return EXIT_SUCCESS;
}
