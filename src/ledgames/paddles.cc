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
#include "ball_sprite.hh"
#include "brick_sprite.hh"

static controls_t *player_controls[3];
static int player_score[2];
static int number_players;
static int serving_player;

static sprite_t paddle_sprite[2];
static ball_sprite_t ball_sprite;

static Mix_Chunk *startup_bong;
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
uint32_t paddle_data[] = {0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF, 0x0000FFFF};

static bool reset_round(void) {
  if ((player_score[0] == 7) || (player_score[1] == 7)) {
    return false;
  }

  if (serving_player == 0) {
    ball_sprite.set_position(paddle_sprite[0].x_+4,58);
    ball_sprite.set_speed(ball_horizontal_speeds[rand()%7], -0.5f);
  } else {
    ball_sprite.set_position(paddle_sprite[1].x_+4,6);
    ball_sprite.set_speed(ball_horizontal_speeds[rand()%7], 0.5f);
  }
  ball_sprite.set_image(1,1,ball_data);
  
  game_state = game_state_t::Serving;

  return true;
}

static void reset_game(int with_number_players) {
  paddle_sprite[0].set_active(true);
  paddle_sprite[0].set_position(28, 60);
  paddle_sprite[0].set_image(8,3,paddle_data);
  
  paddle_sprite[1].set_active(true);
  paddle_sprite[1].set_position(28, 2);
  paddle_sprite[1].set_image(8,3,paddle_data);
  
  player_score[0] = 0;
  player_score[1] = 0;

  serving_player = 0;
  
  number_players = with_number_players;
  
  reset_round();

  printf("\n\n\nGAME START\n\n\n");
}

void init_attract(void) {
  paddle_sprite[0].set_active(false);
  paddle_sprite[1].set_active(false);
  
  ball_sprite.set_position(32,32);
  ball_sprite.set_speed(0.5f, 0.5f);
  ball_sprite.set_image(1,1,ball_data);

  game_state = game_state_t::Attract;
}

void render_game(Screen *screen) {
  screen->set_flip(false);

  screen->set_background_color(0x00000010);
  screen->draw_start();

  if (game_state == game_state_t::Attract) {
    screen->draw_text(32,2,0x00808080,"GAME OVER!");
    screen->draw_text(40,2,0x00808080,"PRESS PLYR");
    screen->draw_text(48,2,0x00808080,"  1 OR 2  ");
  }
  char score[7];
  sprintf(score, "%1d", player_score[1]);
  screen->draw_text(0,59,0x00808080,score,number_players==2);
  sprintf(score, "%1d", player_score[0]);
  screen->draw_text(57,59,0x00808080,score);
  
  paddle_sprite[0].draw_onto(screen);
  paddle_sprite[1].draw_onto(screen);
  ball_sprite.draw_onto(screen);

  screen->draw_end();
  
  player_controls[0]->refresh_status();
  player_controls[1]->refresh_status();
  player_controls[2]->refresh_status();
  
  if (game_state == game_state_t::Attract) {
    if (player_controls[2]->is_pressed(button_a)) {
      reset_game(1);
    } else if (player_controls[2]->is_pressed(button_b)) {
      reset_game(2);
    }
    return;
  } else {
    for (int player = 0; player < number_players; player++) {
      if (player_controls[player]->is_pressed(joystick_left)) {
	if (paddle_sprite[player].x_ > 0) {
	  paddle_sprite[player].x_--;
	}
      }
    
      if (player_controls[player]->is_pressed(joystick_right)) {
	if (paddle_sprite[player].x_ < 56) {
	  paddle_sprite[player].x_++;
	}
      }
    }
    if (game_state == game_state_t::Serving) {
      ball_sprite.x_ = paddle_sprite[serving_player].x_ + 4;
      if (player_controls[serving_player]->is_pressed(button_a)) {
	game_state = game_state_t::Playing;
      }
    }

    if (number_players == 1) {
      float computer_paddle_x_target = ball_sprite.x_ - 4;

      if (computer_paddle_x_target > paddle_sprite[1].x_) paddle_sprite[1].x_ += 0.8f;
      if (computer_paddle_x_target < paddle_sprite[1].x_) paddle_sprite[1].x_ -= 0.8f;

      if (paddle_sprite[1].x_ < 0) { paddle_sprite[1].x_ = 0; }
      if (paddle_sprite[1].x_ > 56) { paddle_sprite[1].x_ = 56; }
    }
  }

  // Move ball
  if (game_state != game_state_t::Serving) {
    ball_sprite.move_sprite();
  }
  
  if (ball_sprite.test_collision(paddle_sprite[0])) {
    ball_sprite.y_ = 59.0f;
    ball_sprite.dy_ = -ball_sprite.dy_ * 1.1f;
    if (ball_sprite.dy_ < -2.0f) ball_sprite.dy_ = -2.0f;
    ball_sprite.dx_ = ball_horizontal_speeds[(int)(ball_sprite.x_ - (paddle_sprite[0].x_ - 1))];
    Mix_PlayChannel(-1, paddle_blip, 0);
  }

  if (ball_sprite.test_collision(paddle_sprite[1])) {
    ball_sprite.y_ = 5.0f;
    ball_sprite.dy_ = -ball_sprite.dy_ * 1.1f;
    if (ball_sprite.dy_ > 2.0f) ball_sprite.dy_ = 2.0f;
    ball_sprite.dx_ = ball_horizontal_speeds[(int)(ball_sprite.x_ - (paddle_sprite[1].x_ - 1))];
    Mix_PlayChannel(-1, paddle_blip, 0);
  }

  if (ball_sprite.y_ >= 63.0f) {
    if (game_state != game_state_t::Attract) {
      player_score[1]++;
      if (number_players == 2) {
	serving_player = 1;
      } else {
	serving_player = 0;
      }
      if (!reset_round()) {
	init_attract();
      }
      return;
    } else {
      ball_sprite.dy_ = -ball_sprite.dy_;
      ball_sprite.dx_ = ball_horizontal_speeds[rand()%7];
    }
  }

  if (ball_sprite.y_ <= 0.0f) {
    if (game_state != game_state_t::Attract) {
      player_score[0]++;
      serving_player = 0;
      if (!reset_round()) {
	init_attract();
      }
      return;
    } else {
      ball_sprite.dy_ = -ball_sprite.dy_;
      ball_sprite.dx_ = ball_horizontal_speeds[rand()%7];
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

  paddle_blip = Mix_LoadWAV("bin/blip2.wav");
  
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
