#include "display.h"
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

const int WINDOW_HEIGHT = 144;
const int WINDOW_WIDTH = 160;

SDL_Window* main_window;
SDL_Renderer* main_renderer;

SDL_Event e;

void display_init(){
    main_window = SDL_CreateWindow(
        "gameboy",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL
    );

    if(main_window == NULL){
        printf("unable to create window");
        SDL_Quit();
        return ;
    }

    main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_PRESENTVSYNC);
}

void sdl_shutdown(){
    if(main_window != NULL){
        SDL_DestroyWindow(main_window);
        main_window = NULL;
    }

    if(main_renderer != NULL){
        SDL_DestroyRenderer(main_renderer);
        main_renderer = NULL;
    }

    SDL_Quit();
}

int render_display(){
    static int running = 1;

    //check for escape key to close window
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = 0;
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }
    }
    if(running){
        SDL_RenderPresent(main_renderer); 
    }
    return running;
}
