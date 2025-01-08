#include "debugger.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "include/nuklear_sdl_renderer.h"

// https://behova.net/posts/nuklear-setup/

struct nk_context *ctx;
struct nk_color background;

void init_nuklear(SDL_Renderer* r, SDL_Window *w){
    ctx = nk_sdl_init(w, r);

    struct nk_font_atlas *atlas;
    struct nk_font_config config = nk_font_config(0);
    struct nk_font *font;

    nk_sdl_font_stash_begin(&atlas);
    font = nk_font_atlas_add_default(atlas, 13.0f, &config);
    nk_sdl_font_stash_end();
    nk_style_set_font(ctx, &font->handle);

    // Set a default background color
    background = nk_rgb(28, 48, 62);
}

void update_debugger(SDL_Event *e){
    // Start Nuklear window and add widgets
    if(nk_begin(ctx, "Example", nk_rect(10, 10, 200, 200), NK_WINDOW_TITLE | NK_WINDOW_MOVABLE )){

    }
    nk_end(ctx);

    // Render the Nuklear commands using OpenGL or SDL
    nk_sdl_render(NK_ANTI_ALIASING_ON);
}

void start_nk_input() {
    nk_input_begin(ctx);
}

void end_nk_input(){
    nk_sdl_handle_grab();
    nk_input_end(ctx);
}
