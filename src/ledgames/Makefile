#########
#
# Build the various demos
#
#
BIN-y += bricks
BIN-y += black
BIN-y += white
BIN-y += paddles
BIN-y += invaders
BIN-y += menu

bricks.srcs += bricks.cc gpio_pin.cc controls.cc screen.cc sprite.cc ball_sprite.cc
black.srcs += black.c
white.srcs += white.c
paddles.srcs += paddles.cc gpio_pin.cc controls.cc screen.cc sprite.cc ball_sprite.cc
invaders.srcs += invaders.cc gpio_pin.cc controls.cc screen.cc sprite.cc invader_sprite.cc ship_sprite.cc png.cc
menu.srcs += menu.cc gpio_pin.cc controls.cc screen.cc sprite.cc png.cc

include ../../Makefile.common

