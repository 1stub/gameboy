#ifndef PPU_H
#define PPU_H

#include "mmu.h"
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

typedef struct{
    ppu_state state;
    int scanline_counter;
}PPU;

extern PPU ppu;

extern void ppu_init();
extern void update_graphics(int cycles);

#endif
