#ifndef PPU_H
#define PPU_H

#include "mmu.h"
#include "display.h"
#include "interrupt.h"

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

typedef enum{
    Fetch_Tile_NO,
    Fetch_Tile_Low,
    Fetch_Tile_High,
    FFIO_Push 
}fetcher_state;


typedef struct{
    ppu_state ppu_state;
    fetcher_state fetcher;
    int scanline_counter;
    int cur_pixel;
    int color_palette[4];
}PPU;

extern PPU ppu;

extern void ppu_init();
extern int update_graphics(int cycles);

#endif
