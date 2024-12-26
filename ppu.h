#ifndef PPU_H
#define PPU_H

#include "mmu.h"
#include "display.h"
#include "interrupt.h"
#include "util.h"

// Our ppu scanlines take 456 T-Cycles to complete
// 456 -> 456 - 80 == OAM Search
// 456 - 80 -> 456 - 80 - 172 == Pixel Transfer
// 456 - 80 - 172 -> 0 == H-Blank
// If LY is > 144 then we reached the max number of 
// posible scanlines and need to enter vblank

typedef enum{
    OAM_Search,
    Pixel_Transfer,
    HBlank,
    VBlank
}ppu_state;

typedef struct{
    ppu_state state;
    int cycles;
    int color_palette[4];
    int update_display;
    int is_window;
    uint32_t pixel_buffer[WINDOW_WIDTH][WINDOW_HEIGHT];
}PPU;

extern PPU ppu;

typedef enum{
    Fetch_Tile_NO,
    Fetch_Tile_Low,
    Fetch_Tile_High,
    FIFO_Push 
}fetcher_state;

typedef struct{
    fetcher_state state;
    int is_signed;
    int cur_pixel;
    word tile_data_bp;
    word tile_map_bp;
    byte cur_tile_x;
    byte cur_tile_y;
    byte tile_no;
    word tile_loc;
    word tile_low;
    word tile_high;
}FETCHER;

extern void ppu_init();
extern int update_graphics(int cycles);

#endif
