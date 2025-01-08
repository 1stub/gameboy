#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <SDL2/SDL.h>

extern void init_nuklear(SDL_Renderer* r, SDL_Window* w);
extern void update_debugger(SDL_Event *e);
extern void start_nk_input();
extern void end_nk_input();

#endif
