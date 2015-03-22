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
#include <unistd.h>

#include <vector>

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#include "ledscape.h"

#include "controls.hh"
#include "screen.hh"
#include "sprite.hh"
#include "invader_sprite.hh"
#include "png.hh"

static controls_t *player_controls[3];
static int player_score[2];
static int player_lives[2];
static int number_players;
static int current_player;

static sprite_t ship_sprite;
static sprite_t ship_missile_sprite;
static std::vector<invader_sprite_t> invader_sprites[2];

static Mix_Chunk *startup_bong;
static Mix_Chunk *wall_blip;
static Mix_Chunk *block_blip[3];
static Mix_Chunk *paddle_blip;

static png_t sprite_sheet;

enum class game_state_t {
	Attract,
	Serving,
	Playing,
	Resetting,
};
  
static game_state_t game_state;
static bool invaders_right;
static bool shot_fired;

static void reset_invaders(int for_player) {
	invader_sprites[for_player].clear();
	invaders_right = true;
	for (int row_counter = 0; row_counter < 4; row_counter++) {
		for (int column_counter = 0; column_counter < 6; column_counter++) {
			invader_sprite_t invader_sprite;
			invader_sprite.set_active(true);
			invader_sprite.set_speed(0.05, 0.0);
			invader_sprite.set_position(column_counter * 10, 10 + (row_counter * 8));
			invader_sprite.set_image(row_counter * 28,0,7,7,&sprite_sheet, 0);
			invader_sprite.set_image((row_counter * 28) + 7,0,7,7,&sprite_sheet, 1);
			invader_sprites[for_player].push_back(invader_sprite);
		}
	}
}

static bool reset_round(void) {
	current_player = (current_player + 1) % number_players;
	if (player_lives[current_player] == 0) {
		return false;
	}
  
  	shot_fired = false;
	ship_missile_sprite.set_active(false);
	game_state = game_state_t::Serving;

	return true;
}

static void reset_game(int with_number_players) {
	ship_sprite.set_active(true);
	ship_sprite.set_position(28, 55);
	ship_sprite.set_image(168,0,7,7,&sprite_sheet);
  
	reset_invaders(0);
	reset_invaders(1);
  
	player_score[0] = 0;
	player_score[1] = 0;
  
	player_lives[0] = 3;
	player_lives[1] = 3;
  
	number_players = with_number_players;
	current_player = 1;
  
	reset_round();

	game_state = game_state_t::Playing;

	printf("\n\n\nGAME START\n\n\n");
}

void init_attract(void) {
	ship_sprite.set_active(false);
  
	current_player = 0;
	reset_invaders(0);
  
	game_state = game_state_t::Attract;
}

void render_game(Screen *screen) {
	if (current_player == 1) {
		screen->set_flip(true);
	} else {
		screen->set_flip(false);
	}
	screen->set_background_color(0x00000000);
	screen->draw_start();

	ship_sprite.draw_onto(screen);
	ship_missile_sprite.draw_onto(screen);
	ship_missile_sprite.move_sprite();
	if (ship_missile_sprite.get_y_position() < 0.0f) {
		ship_missile_sprite.set_active(false);
	}

	bool change_direction = false;
	for (auto &invader_sprite : invader_sprites[current_player]) {
		invader_sprite.move_sprite();
		if (invader_sprite.is_active()) {
			if ((invaders_right && (invader_sprite.get_x_position() > 57)) || (!invaders_right && (invader_sprite.get_x_position() < 1))) {
				change_direction = true;
			}
		}
	}
	
	if (change_direction) {
		if (invaders_right) {
			invaders_right = false;
			for (auto &invader_sprite : invader_sprites[current_player]) {
				invader_sprite.set_speed(-0.05, 0);
				if (game_state == game_state_t::Playing) {
					invader_sprite.set_position(invader_sprite.get_x_position(), invader_sprite.get_y_position()+1);
				}
			}			
		} else {
			invaders_right = true;
			for (auto &invader_sprite : invader_sprites[current_player]) {
				invader_sprite.set_speed(0.05, 0);
				if (game_state == game_state_t::Playing) {
					invader_sprite.set_position(invader_sprite.get_x_position(), invader_sprite.get_y_position()+1);
				}
			}			
		}
	}

	for (auto &invader_sprite : invader_sprites[current_player]) {
		invader_sprite.draw_onto(screen);
	}

	if (game_state == game_state_t::Attract) {
		screen->draw_text(32,2,0x00808080,"GAME OVER!");
		screen->draw_text(40,2,0x00808080,"PRESS PLYR");
		screen->draw_text(48,2,0x00808080,"  1 OR 2  ");
	} else {
		char score[7];
		char lives[1];
		sprintf(score, "%06d", player_score[current_player]);
		screen->draw_text(0,2,0x00808080,score);
		sprintf(lives, "%01d", player_lives[current_player]);
		screen->draw_text(0,48,0x00808080,lives);

	}
  
	screen->draw_end();
  
	player_controls[current_player]->refresh_status();
  
	if (game_state == game_state_t::Attract) {
		player_controls[2]->refresh_status();
		if (player_controls[2]->is_pressed(button_a)) {
			reset_game(1);
		} else if (player_controls[2]->is_pressed(button_b)) {
			reset_game(2);
		}
		return;
	} else {
		if (player_controls[current_player]->is_pressed(joystick_left)) {
			if (ship_sprite.x_ > 0) {
				ship_sprite.x_--;
			}
		}
    
		if (player_controls[current_player]->is_pressed(joystick_right)) {
			if (ship_sprite.x_ < 57) {
				ship_sprite.x_++;
			}
		}

		if ((!shot_fired) && (player_controls[current_player]->is_pressed(button_a))) {
			ship_missile_sprite.set_active(true);
			ship_missile_sprite.set_position(ship_sprite.get_x_position(), 51);
			ship_missile_sprite.set_image(196,0,7,7,&sprite_sheet);
			ship_missile_sprite.set_speed(0.0f, -2.0f);
			shot_fired = true;
		} 
		if (!player_controls[current_player]->is_pressed(button_a)) {
			shot_fired = false;
		}
	}

	if (game_state == game_state_t::Playing) {
		bool active_invaders = false;
		for (auto &invader_sprite : invader_sprites[current_player]) {
			active_invaders |= invader_sprite.is_active();
	        if (invader_sprite.test_collision(ship_missile_sprite)) {
	  			invader_sprite.set_active(false);
				ship_missile_sprite.set_active(false);
			}
		}
		if (!active_invaders) {
			reset_invaders(current_player);
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

	startup_bong = Mix_LoadWAV("/root/startup.wav");
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

	ledscape_t * const leds = ledscape_init(config, 0);

	sprite_sheet.read_file("/root/Invaders.png");

	player_controls[0] = new controls_t(1);
	player_controls[1] = new controls_t(2);
	player_controls[2] = new controls_t(3);
  
	init_attract();
  
	printf("init done\n");
	uint32_t * const p = (uint32_t*)calloc(width*height,4);

	Screen *screen = new Screen(leds, p);
  
	init_sdl();

	wall_blip = Mix_LoadWAV("/root/blip1.wav");
	paddle_blip = Mix_LoadWAV("/root/blip2.wav");
	block_blip[0] = Mix_LoadWAV("/root/blip3.wav");
	block_blip[1] = Mix_LoadWAV("/root/blip4.wav");
	block_blip[2] = Mix_LoadWAV("/root/blip5.wav");
  
	while (1)
	{
		render_game(screen);

		usleep(20000);

	}

	ledscape_close(leds);

	return EXIT_SUCCESS;
}
