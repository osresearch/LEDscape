/** \file
* Invaders sample game
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
#include "invader_sprite.hh"
#include "ship_sprite.hh"
#include "png.hh"

static controls_t *player_controls[3];
static int player_score[2];
static int player_lives[2];
static int number_players;
static int current_player;

static ship_sprite_t ship_sprite;
static sprite_t ship_missile_sprite;
static std::vector<invader_sprite_t> invader_sprites[2];
static sprite_t invader_missile_sprite;
static int32_t lowest_invaders[6];

static Mix_Chunk *startup_bong;
static Mix_Chunk *invader_explosion;
static Mix_Chunk *ship_explosion;
static Mix_Chunk *invader_march[4];

static png_t sprite_sheet;

enum class game_state_t {
	Attract,
	Serving,
	Playing,
	Touchdown,
	NewShip,
};
  
static game_state_t game_state;
static bool invaders_right;
static bool shot_fired;
static float invader_speed;
static uint32_t frames_in_state;
static uint32_t frames_till_shot;

static uint32_t invader_march_state;
static uint32_t invader_march_frames;

static std::default_random_engine generator;
static std::uniform_int_distribution<unsigned int> invader_distribution(0,5);
static std::uniform_int_distribution<unsigned int> shot_distribution(65,95);

static void reset_invaders(int for_player) {
	invader_sprites[for_player].clear();
	invaders_right = true;
	invader_speed = 0.05f;
	for (int row_counter = 0; row_counter < 4; row_counter++) {
		for (int column_counter = 0; column_counter < 6; column_counter++) {
			invader_sprite_t invader_sprite((5 - row_counter) * 100);
			invader_sprite.set_active(true);
			invader_sprite.set_speed(invader_speed, 0.0);
			invader_sprite.set_position(column_counter * 10, 10 + (row_counter * 8));
			invader_sprite.set_image(row_counter * 28,0,7,7,&sprite_sheet, 0);
			invader_sprite.set_image((row_counter * 28) + 7,0,7,7,&sprite_sheet, 1);
			invader_sprite.set_image((row_counter * 28) + 14,0,7,7,&sprite_sheet, 2);
			invader_sprite.set_image((row_counter * 28) + 21,0,7,7,&sprite_sheet, 3);
			invader_sprites[for_player].push_back(invader_sprite);
		}
	}
	lowest_invaders[0] = 18;
	lowest_invaders[1] = 19;
	lowest_invaders[2] = 20;
	lowest_invaders[3] = 21;
	lowest_invaders[4] = 22;
	lowest_invaders[5] = 23;
}

static bool reset_round(void) {
	uint32_t next_player = (current_player + 1) % number_players;
	if (player_lives[next_player] > 0) {
		current_player = next_player;
	}
	if ((player_lives[0] == 0) && (player_lives[1] == 0)) {
		return false;
	}
  
	shot_fired = false;
	ship_missile_sprite.set_active(false);
	invader_missile_sprite.set_active(false);
	ship_sprite.set_active(true);
	game_state = game_state_t::Playing;
	
	frames_till_shot = 60;
	
	invader_march_state = 0;
	invader_march_frames = 0;

	return true;
}

static void reset_game(int with_number_players) {
	ship_sprite.set_active(true);
	ship_sprite.set_position(28, 55);
	ship_sprite.set_image(168,0,7,7,&sprite_sheet, 0);
	ship_sprite.set_image(168+7,0,7,7,&sprite_sheet, 1);
	ship_sprite.set_image(168+14,0,7,7,&sprite_sheet, 2);
	ship_sprite.set_image(168+28,0,7,7,&sprite_sheet, 3);
  
	reset_invaders(0);
	reset_invaders(1);
  
	player_score[0] = 0;
	player_score[1] = 0;
  
	player_lives[0] = 3;
	if (with_number_players == 2) {
		player_lives[1] = 3;
	} else {
		player_lives[1] = 0;
	}
  
	number_players = with_number_players;
	current_player = 1;
  
	reset_round();

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
	invader_missile_sprite.draw_onto(screen);
	ship_missile_sprite.move_sprite();
	invader_missile_sprite.move_sprite();
	if (ship_missile_sprite.get_y_position() < 0.0f) {
		ship_missile_sprite.set_active(false);
	}

	bool change_direction = false;
	bool invader_touchdown = false;
	if ((game_state == game_state_t::Attract) || (game_state == game_state_t::Playing)) {
		for (auto &invader_sprite : invader_sprites[current_player]) {
			invader_sprite.move_sprite();
			if (invader_sprite.is_active()) {
				if ((invaders_right && (invader_sprite.get_x_position() > 57.5f)) || (!invaders_right && (invader_sprite.get_x_position() < 0.5f))) {
					change_direction = true;
				}
				if (invader_sprite.get_y_position() > 47) {
					invader_touchdown = true;
				}
			}
		}
	}

	if (invader_touchdown) {
		game_state = game_state_t::Touchdown;
		Mix_PlayChannel(-1, ship_explosion, 0);
		frames_in_state = 0;
		ship_sprite.set_active(false);
		player_lives[current_player] = 0;
		for (auto &invader_sprite : invader_sprites[current_player]) {
			invader_sprite.set_position(invader_sprite.get_x_position(), invader_sprite.get_y_position()+8);
		}
	}
	
	if (change_direction) {
		invaders_right = !invaders_right;
	}

	for (auto &invader_sprite : invader_sprites[current_player]) {
		invader_sprite.draw_onto(screen);
	}
	
	frames_till_shot--;
	if (frames_till_shot == 0) {
		invader_missile_sprite.set_active(true);
		int32_t invader_column = -1;
		do {
			invader_column = lowest_invaders[invader_distribution(generator)];
		} while (invader_column < 0);
		sprite_t invader_sprite = invader_sprites[current_player][invader_column];
		invader_missile_sprite.set_position(invader_sprite.get_x_position(), invader_sprite.get_y_position());
		invader_missile_sprite.set_image(196,0,7,7,&sprite_sheet);
		invader_missile_sprite.set_speed(0.0f, 0.8f);
		frames_till_shot=shot_distribution(generator);
	}

	if (game_state == game_state_t::Attract) {
		screen->draw_text(0,2,0x00808080,"GAME OVER!");
		screen->draw_text(48,2,0x00808080,"PRESS PLYR");
		screen->draw_text(56,2,0x00808080,"  1 OR 2  ");
	} else if ((game_state == game_state_t::Playing) || (game_state == game_state_t::NewShip)) {
		char score[7];
		char lives[1];
		sprintf(score, "%06d", player_score[current_player]);
		screen->draw_text(0,2,0x00808080,score);
		sprintf(lives, "%01d", player_lives[current_player]);
		screen->draw_text(0,48,0x00808080,lives);
	} else if (game_state == game_state_t::Touchdown) {
		char score[7];
		char lives[1];
		sprintf(score, "%06d", player_score[current_player]);
		screen->draw_text(0,2,0x00808080,score);
		sprintf(lives, "%01d", player_lives[current_player]);
		screen->draw_text(0,48,0x00808080,lives);
		screen->draw_text(32,2,0x00808080,"GAME OVER!");
	}
  
	screen->draw_end();
  
	player_controls[current_player]->refresh_status();
	player_controls[2]->refresh_status();
  
	if (game_state == game_state_t::Attract) {
		if (player_controls[2]->is_pressed(button_a)) {
			reset_game(1);
		} else if (player_controls[2]->is_pressed(button_b)) {
			reset_game(2);
		}
	} else if (game_state == game_state_t::Playing) {
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

	if ((game_state == game_state_t::Attract) || (game_state == game_state_t::Playing)) {
		int active_invaders = 0;
		int32_t invader_idx = 0;
		for (auto &invader_sprite : invader_sprites[current_player]) {
			active_invaders += (invader_sprite.is_active() ? 1 : 0);
			if (invader_sprite.test_collision(ship_missile_sprite, false)) {
				Mix_PlayChannel(-1, invader_explosion, 0);
				player_score[current_player] += invader_sprite.destroy_sprite();
				ship_missile_sprite.set_active(false);
				for (uint8_t idx_ctr = 0; idx_ctr < 6; idx_ctr++) {
					if (lowest_invaders[idx_ctr] == invader_idx) {
						// Player just shot the lowest invader in a column.
						lowest_invaders[idx_ctr] -= 6;
					}
				}
			}
			invader_idx++;
		}
		invader_speed = 0.06f + ((24 - active_invaders) * 0.05f);
		if (active_invaders == 0) {
			reset_invaders(current_player);
		}
		
		if (game_state == game_state_t::Playing) {
			if (invader_march_frames == 0) {
				Mix_PlayChannel(-1, invader_march[invader_march_state], 0);
				invader_march_state = (invader_march_state + 1) % 4;
				invader_march_frames = 10 + (active_invaders/4);
			} else {
				invader_march_frames--;
			}
		}

		if (ship_sprite.test_collision(invader_missile_sprite, false)) {
			Mix_PlayChannel(-1, ship_explosion, 0);
			game_state = game_state_t::NewShip;
			ship_sprite.destroy_sprite();
			player_lives[current_player]--;
			frames_in_state = 0;
		}
	}

	for (auto &invader_sprite : invader_sprites[current_player]) {
		invader_sprite.set_speed(invader_speed * (invaders_right ? 1 : -1), 0);
		if (change_direction) {
			if (game_state == game_state_t::Playing) {
				invader_sprite.set_position(invader_sprite.get_x_position(), invader_sprite.get_y_position()+1);
			}
		}
	}	
	
	if (game_state == game_state_t::Touchdown) {
		frames_in_state++;
		if (frames_in_state == 120) {
			if (!reset_round()) {
				init_attract();
			}
		}
	}		

	if (game_state == game_state_t::NewShip) {
		frames_in_state++;
		if (frames_in_state == 120) {
			if (!reset_round()) {
				init_attract();
			}
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

	ledscape_t * const leds = ledscape_init(config, 0);

	sprite_sheet.read_file("bin/Invaders.png");

	player_controls[0] = new controls_t(1);
	player_controls[1] = new controls_t(2);
	player_controls[2] = new controls_t(3);
  
	init_attract();
  
	printf("init done\n");
	uint32_t * const p = (uint32_t*)calloc(width*height,4);

	Screen *screen = new Screen(leds, p);
  
	init_sdl();

	invader_explosion = Mix_LoadWAV("bin/invader_explosion.wav");
	ship_explosion = Mix_LoadWAV("bin/ship_explosion.wav");
	invader_march[0] = Mix_LoadWAV("bin/invader_march1.wav");
	invader_march[1] = Mix_LoadWAV("bin/invader_march2.wav");
	invader_march[2] = Mix_LoadWAV("bin/invader_march3.wav");
	invader_march[3] = Mix_LoadWAV("bin/invader_march4.wav");
  
	try {
		while (1)
		{
			render_game(screen);

			usleep(20000);

		}
	}
	catch (control_exit_exception* ex) {
		delete screen;
	}

	return EXIT_SUCCESS;
}
