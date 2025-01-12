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

#define LCDC_LINE_CYCLES 456

//each ppu mode takes these number of cycles
#define LCDC_MODE_OAM_CYCLES 80
#define LCDC_MODE_PIXEL_CYCLES 172
#define LCDC_MODE_HBLANK_CYCLES 204

//these values correspond to bits 0 and 1 in STAT
#define OAM_NUM 0x02
#define PIXEL_TRANSFER_NUM 0x03
#define HBLANK_NUM 0x00
#define VBLANK_NUM 0x01

//actual value of LY
#define LY_VAL read(LY)

//checks if the flag is set, if it is request LCD interrupt
#define DO_STAT_INTERRUPTS(state)       \
    do{                                 \
        if(read(STAT) & (1 << state)){  \
            request_interrupt(1);       \
        }                               \
    }while(0)

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
    int should_update_display;
    int is_window;
    uint32_t pixel_buffer[WINDOW_WIDTH][WINDOW_HEIGHT];
}PPU;

typedef struct{
    int tile_data_bp;
    int tile_map_bp;
    int is_signed;
} TileOffsets;

extern PPU ppu;

extern void ppu_init();
extern int update_graphics(int cycles);

#endif
