#include "display.h"
#include "debugger.h"

SDL_Window* main_window;
SDL_Renderer* main_renderer;
SDL_Texture* display_texture;

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

    display_texture = SDL_CreateTexture(
        main_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_WIDTH,
        WINDOW_HEIGHT
    );

    init_nuklear(main_renderer, main_window); 
}

void sdl_shutdown(){
    if(display_texture != NULL){
        SDL_DestroyTexture(display_texture);
        display_texture = NULL;
    }

    if(main_renderer != NULL){
        SDL_DestroyRenderer(main_renderer);
        main_renderer = NULL;
    }

    if(main_window != NULL){
        SDL_DestroyWindow(main_window);
        main_window = NULL;
    }
}

void update_display_buffer(uint32_t pixel_buffer[WINDOW_WIDTH][WINDOW_HEIGHT]){
    SDL_UpdateTexture(display_texture, NULL, pixel_buffer, WINDOW_WIDTH * sizeof(uint32_t));
}

int render_display(){
    static int running = 1;

    //check for escape key to close window
    start_nk_input();
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = 0;
        } else if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                running = 0;
            }
        }
    }
    end_nk_input();

    if(running){
        SDL_RenderClear(main_renderer);
        SDL_RenderCopy(main_renderer, display_texture, NULL, NULL);

        update_debugger(&e);
        SDL_RenderPresent(main_renderer);
    }
    return running;
}
