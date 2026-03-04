#pragma once

#include <stdint.h>

#ifdef __cplusplus
using uint24_t = uint32_t;
#else
typedef uint32_t uint24_t;
#endif

#ifndef WIDTH
#define WIDTH 128
#endif
#ifndef HEIGHT
#define HEIGHT 64
#endif

#ifndef UP_BUTTON
#define UP_BUTTON    0x01
#define DOWN_BUTTON  0x02
#define LEFT_BUTTON  0x04
#define RIGHT_BUTTON 0x08
#define A_BUTTON     0x10
#define B_BUTTON     0x20
#endif

#ifndef INPUT_UP
#define INPUT_UP UP_BUTTON
#endif
#ifndef INPUT_DOWN
#define INPUT_DOWN DOWN_BUTTON
#endif
#ifndef INPUT_LEFT
#define INPUT_LEFT LEFT_BUTTON
#endif
#ifndef INPUT_RIGHT
#define INPUT_RIGHT RIGHT_BUTTON
#endif
#ifndef INPUT_A
#define INPUT_A A_BUTTON
#endif
#ifndef INPUT_B
#define INPUT_B B_BUTTON
#endif

#define ARDUBOY_LIB_VER               60000
#define ARDUBOY_UNIT_NAME_LEN         6
#define ARDUBOY_UNIT_NAME_BUFFER_SIZE (ARDUBOY_UNIT_NAME_LEN + 1)
#define BLACK                         1
#define WHITE                         0
#define INVERT                        2
#define CLEAR_BUFFER                  true
#define RED_LED                       1
#define GREEN_LED                     2
#define BLUE_LED                      4
#define ARDUBOY_NO_USB
