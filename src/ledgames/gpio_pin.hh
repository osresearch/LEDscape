#include <stdint.h>

class gpio_pin_t {

public:
  gpio_pin_t (uint8_t button_number);
  ~gpio_pin_t();
  
  bool is_pressed();

private:
  void export_gpio();

  uint8_t button_number_;
  int file_descriptor_;
};
