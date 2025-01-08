#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL2/SDL.h>

#include "util.h"

extern void display_init();
extern int render_display();
extern void update_display_buffer(uint32_t pixel_buffer[WINDOW_WIDTH][WINDOW_HEIGHT]);

#endif
