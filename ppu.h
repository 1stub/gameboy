#ifndef PPU_H
#define PPU_H

#include "mmu.h"

typedef enum{
    OAM_Search,
    Pixel_Transfer,
    HBlank,
    VBlank
}ppu_state;

typedef struct{
    ppu_state state;
}PPU;

extern PPU ppu;

void update_graphics();
void update_scanline();

#endif
