#include <iostream>

#include "controls.hh"

#define BUTTON_PLAYERSTART_P1 70
#define BUTTON_PLAYERSTART_P2 71
#define BUTTON_P1_JOYUP 79
#define BUTTON_P1_JOYDN 73
#define BUTTON_P1_JOYLEFT 74
#define BUTTON_P1_JOYRIGHT 75
#define BUTTON_P1_ACTION_PRI 76
#define BUTTON_P1_ACTION_SEC 77
#define BUTTON_P2_JOYUP 8
#define BUTTON_P2_JOYDN 10
#define BUTTON_P2_JOYLEFT 9
#define BUTTON_P2_JOYRIGHT 81
#define BUTTON_P2_ACTION_PRI 11
#define BUTTON_P2_ACTION_SEC 80

controls_t::controls_t (uint8_t player_number, bool flip_lr) {
  pin_info_.resize(button_assignments_count);
  if (player_number == 1) {
    pin_info_[joystick_up] = new gpio_pin_t(BUTTON_P1_JOYUP);
    pin_info_[joystick_down] = new gpio_pin_t(BUTTON_P1_JOYDN);
    if (flip_lr) {
      pin_info_[joystick_left] = new gpio_pin_t(BUTTON_P1_JOYRIGHT);
      pin_info_[joystick_right] = new gpio_pin_t(BUTTON_P1_JOYLEFT);
    } else {
      pin_info_[joystick_left] = new gpio_pin_t(BUTTON_P1_JOYLEFT);
      pin_info_[joystick_right] = new gpio_pin_t(BUTTON_P1_JOYRIGHT);
    }
    pin_info_[button_a] = new gpio_pin_t(BUTTON_P1_ACTION_PRI);
    pin_info_[button_b] = new gpio_pin_t(BUTTON_P1_ACTION_SEC);
  } else if (player_number == 2) {
    pin_info_[joystick_up] = new gpio_pin_t(BUTTON_P2_JOYUP);
    pin_info_[joystick_down] = new gpio_pin_t(BUTTON_P2_JOYDN);
    if (flip_lr) {
      pin_info_[joystick_left] = new gpio_pin_t(BUTTON_P2_JOYRIGHT);
      pin_info_[joystick_right] = new gpio_pin_t(BUTTON_P2_JOYLEFT);
    } else {
      pin_info_[joystick_left] = new gpio_pin_t(BUTTON_P2_JOYLEFT);
      pin_info_[joystick_right] = new gpio_pin_t(BUTTON_P2_JOYRIGHT);
    }
    pin_info_[button_a] = new gpio_pin_t(BUTTON_P2_ACTION_PRI);
    pin_info_[button_b] = new gpio_pin_t(BUTTON_P2_ACTION_SEC);
  } else if (player_number == 3) {
    pin_info_[joystick_up] = new gpio_pin_t(BUTTON_P1_JOYUP);
    pin_info_[joystick_down] = new gpio_pin_t(BUTTON_P1_JOYDN);
    pin_info_[joystick_left] = new gpio_pin_t(BUTTON_P1_JOYLEFT);
    pin_info_[joystick_right] = new gpio_pin_t(BUTTON_P1_JOYRIGHT);
    pin_info_[button_a] = new gpio_pin_t(BUTTON_PLAYERSTART_P1);
    pin_info_[button_b] = new gpio_pin_t(BUTTON_PLAYERSTART_P2);
  }
}

controls_t::~controls_t () {
}

void controls_t::refresh_status() {
  button_pressed_.clear();
  for (auto pin_info : pin_info_) {
    button_pressed_.push_back(pin_info->is_pressed());
  }
}

bool controls_t::is_pressed(button_assignments_t button) {
  return button_pressed_[button];
}

