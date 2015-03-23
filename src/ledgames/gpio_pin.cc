#include <iostream>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gpio_pin.hh"

gpio_pin_t::gpio_pin_t (uint8_t button_number) :
  button_number_(button_number)
{
  export_gpio();
  
  char button_file_name[1024];
  snprintf(button_file_name, 1024, "/sys/class/gpio/gpio%d/value", button_number);
  file_descriptor_  = open(button_file_name, O_RDONLY);
  if (-1  == file_descriptor_) {
    printf("Failed opening button %d\n", button_number);
    throw 1;
  }
}

gpio_pin_t::~gpio_pin_t () {
  close(file_descriptor_);
}

void gpio_pin_t::export_gpio (void) {
  FILE *exportfile = fopen("/sys/class/gpio/export", "w");
  if (NULL == exportfile) {
    printf("Failed opening export device");
    return;
  }

  fprintf(exportfile, "%d", button_number_);
  fclose(exportfile);
}

bool gpio_pin_t::is_pressed(void) {
  uint8_t button_value;
  lseek(file_descriptor_, 0, SEEK_SET);
  read(file_descriptor_, &button_value, 1);
  return (button_value == '0') ? true : false;
}
