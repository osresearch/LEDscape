#########
#
# Build the various demos
#
#
BIN-y += breakout
BIN-y += black
BIN-y += white
BIN-y += pong
BIN-y += invaders

robin-hood-test.srcs += robin-hood-test.c
breakout.srcs += bricks.cc gpio_pin.cc controls.cc screen.cc sprite.cc ball_sprite.cc
black.srcs += black.c
white.srcs += white.c
pong.srcs += pong.cc gpio_pin.cc controls.cc screen.cc sprite.cc ball_sprite.cc
invaders.srcs += invaders.cc gpio_pin.cc controls.cc screen.cc sprite.cc invader_sprite.cc png.cc

include ../../Makefile.common

