#include <iostream>
#include <exception>
#include <vector>
#include <cstdint>

#include "gpio_pin.hh"

class control_exit_exception : public std::exception{
};

enum button_assignments_t {
  joystick_up,
  joystick_down,
  joystick_left,
  joystick_right,
  button_a,
  button_b,
  button_assignments_count,
};
  
class controls_t {
public:
  controls_t(uint8_t player_number, bool flip_lr = false);
  ~controls_t();

  void refresh_status();
  bool is_pressed(button_assignments_t button);
  
private:
  std::vector<gpio_pin_t*> pin_info_;
  std::vector<bool>button_pressed_;
  uint8_t player_number_;
  uint32_t exit_pressed_frames_;
};

