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
    ppu_state ppu_state;
    int scanline_counter;
    int color_palette[4];
    int update_display;
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
    byte cycle_counter;
    int cur_pixel;
    int cur_tile_no;
    word cur_tile_data_low;
    word cur_tile_data_high;
    word cur_tile_data_address;
}FETCHER;

extern void ppu_init();
extern int update_graphics(int cycles);

#endif
