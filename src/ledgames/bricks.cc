/** \file
 * Bricks sample game
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
#include "ball_sprite.hh"
#include "brick_sprite.hh"

static controls_t *player_controls[3];
static int player_score[2];
static int player_lives[2];
static int number_players;
static int current_player;

static sprite_t paddle_sprite;
static ball_sprite_t ball_sprite;
static std::vector<brick_sprite_t> brick_sprites[2];

static Mix_Chunk *startup_bong;
static Mix_Chunk *wall_blip;
static Mix_Chunk *block_blip[3];
static Mix_Chunk *paddle_blip;

enum class game_state_t {
  Attract,
  Serving,
    Playing,
    Resetting,
};
  
static game_state_t game_state;

static float ball_horizontal_speeds[] = {
  -1.1f,
  -0.9f,
  -0.7f,
  -0.5f,
  0.3f,
  0.5f,
  0.7f,
  0.9f,
  1.1f
};

uint32_t ball_data[] = {0x00808000, 0x00808000, 0x00808000, 0x00808000};
uint32_t paddle_data[] = {0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF};
uint32_t brick_data[][8] = {
  {0x00800000, 0x00800000, 0x00800000, 0x00100000, 0x00800000, 0x00800000, 0x00800000, 0x00100000},
  {0x00008000, 0x00008000, 0x00008000, 0x00001000, 0x00008000, 0x00008000, 0x00008000, 0x00001000},
  {0x00004080, 0x00004080, 0x00004080, 0x00000A10, 0x00004080, 0x00004080, 0x00004080, 0x00000A10},
  {0x00804000, 0x00804000, 0x00804000, 0x00100A00, 0x00804000, 0x00804000, 0x00804000, 0x00100A00},
  {0x00800040, 0x00800040, 0x00800040, 0x0010000A, 0x00800040, 0x00800040, 0x00800040, 0x0010000A},
  {0x00204010, 0x00204010, 0x00204010, 0x0010200A, 0x00204010, 0x00204010, 0x00204010, 0x0010200A},
};

static void reset_bricks(int for_player) {
  brick_sprites[for_player].clear();
  for (int row_counter = 0; row_counter < 6; row_counter++) {
    for (int column_counter = 0; column_counter < 16; column_counter++) {
      brick_sprite_t brick_sprite;
      brick_sprite.set_active(true);
      brick_sprite.set_position(column_counter * 4, 10 + (row_counter * 2));
      brick_sprite.set_image(4,2, brick_data[row_counter]);
      brick_sprite.set_score((6 - row_counter) * 10);
      brick_sprite.set_tone_number((5 - row_counter) / 2);
      brick_sprites[for_player].push_back(brick_sprite);
    }
  }
}

static bool reset_round(void) {
  current_player = (current_player + 1) % number_players;
  if (player_lives[current_player] == 0) {
    return false;
  }
  
  ball_sprite.set_position(32,32);
  ball_sprite.set_speed(0.3f, 0.5f);
  ball_sprite.set_image(1,1,ball_data);
  
  game_state = game_state_t::Serving;

  return true;
}

static void reset_game(int with_number_players) {
  paddle_sprite.set_active(true);
  paddle_sprite.set_position(28, 60);
  paddle_sprite.set_image(8,2,paddle_data);
  
  reset_bricks(0);
  reset_bricks(1);
  
  player_score[0] = 0;
  player_score[1] = 0;
  
  player_lives[0] = 3;
  player_lives[1] = 3;
  
  number_players = with_number_players;
  current_player = 1;
  
  reset_round();

  printf("\n\n\nGAME START\n\n\n");
}

void init_attract(void) {
  paddle_sprite.set_active(false);
  
  ball_sprite.set_position(32,32);
  ball_sprite.set_speed(0.5f, 0.5f);
  ball_sprite.set_image(1,1,ball_data);

  current_player = 0;
  reset_bricks(0);
  
  game_state = game_state_t::Attract;
}

void render_game(Screen *screen) {
  if (current_player == 1) {
    screen->set_flip(true);
  } else {
    screen->set_flip(false);
  }
  screen->set_background_color(0x00000010);
  screen->draw_start();

  paddle_sprite.draw_onto(screen);
  ball_sprite.draw_onto(screen);

  for (auto &brick_sprite : brick_sprites[current_player]) {
    brick_sprite.draw_onto(screen);
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
  player_controls[2]->refresh_status();
  
  if (game_state == game_state_t::Attract) {
    if (player_controls[2]->is_pressed(button_a)) {
      reset_game(1);
    } else if (player_controls[2]->is_pressed(button_b)) {
      reset_game(2);
    }
    return;
  } else {
    if (player_controls[current_player]->is_pressed(joystick_left)) {
      if (paddle_sprite.x_ > 0) {
	paddle_sprite.x_--;
      }
    }
    
    if (player_controls[current_player]->is_pressed(joystick_right)) {
      if (paddle_sprite.x_ < 56) {
	paddle_sprite.x_++;
      }
    }
    if (game_state == game_state_t::Serving) {
      if (player_controls[current_player]->is_pressed(button_a)) {
	game_state = game_state_t::Playing;
      }
    }
  }

  // Move ball
  if (game_state != game_state_t::Serving) {
    ball_sprite.move_sprite();
  }
  
  if (ball_sprite.test_collision(paddle_sprite, true)) {
    ball_sprite.y_ = 59.0f;
    ball_sprite.dy_ = -ball_sprite.dy_;
    ball_sprite.dx_ = ball_horizontal_speeds[(int)(ball_sprite.x_ - (paddle_sprite.x_ - 1))];
    Mix_PlayChannel(-1, paddle_blip, 0);
  }

  if (ball_sprite.y_ >= 63.0f) {
    if (game_state != game_state_t::Attract) {
      player_lives[current_player]--;
      if (!reset_round()) {
	init_attract();
      }
      return;
    } else {
      ball_sprite.dy_ = -ball_sprite.dy_;
      ball_sprite.dx_ = ball_horizontal_speeds[rand()%7];
    }
  }

  if (game_state == game_state_t::Playing) {
    bool active_bricks = false;
    for (auto &brick_sprite : brick_sprites[current_player]) {
      active_bricks |= brick_sprite.is_active();
      if (brick_sprite.test_collision(ball_sprite, true)) {
	brick_sprite.set_active(false);
	game_state = game_state_t::Resetting;
	ball_sprite.dy_ = -ball_sprite.dy_;
	player_score[current_player] += brick_sprite.score();
	Mix_PlayChannel(-1, block_blip[brick_sprite.tone_number()], 0);
      }
    }
    if (!active_bricks) {
      reset_bricks(current_player);
    }
  } else {
    if (game_state != game_state_t::Attract) {
      if ((ball_sprite.y_ < 9) || (ball_sprite.y_ > 32)) {
	game_state = game_state_t::Playing;
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
  int audio_buffers = 8192;

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

  player_controls[0] = new controls_t(1);
  player_controls[1] = new controls_t(2);
  player_controls[2] = new controls_t(3);
  
  init_attract();
  
  printf("init done\n");
  uint32_t * const p = (uint32_t*)calloc(width*height,4);

  Screen *screen = new Screen(leds, p);
  
  init_sdl();

  wall_blip = Mix_LoadWAV("bin/blip1.wav");
  paddle_blip = Mix_LoadWAV("bin/blip2.wav");
  block_blip[0] = Mix_LoadWAV("bin/blip3.wav");
  block_blip[1] = Mix_LoadWAV("bin/blip4.wav");
  block_blip[2] = Mix_LoadWAV("bin/blip5.wav");
  
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
