#include "ppu.h"

PPU ppu;

static void cycle_ppu();

void ppu_init(){
    ppu.scanline_counter = 456;

    //eventually would like to use custom palette
    byte palette = read(BGP);
    ppu.color_palette[0] = palette & (0x03);
    ppu.color_palette[1] = palette & (0x0C);
    ppu.color_palette[2] = palette & (0x30);
    ppu.color_palette[3] = palette & (0xC0);

    ppu.ppu_state = OAM_Search;
    ppu.fetcher = Fetch_Tile_NO;
    ppu.cur_pixel = 0;
}

int update_graphics(int cycles){
    cycle_ppu();
    if(read(LCDC) & (1<<7)){
        ppu.scanline_counter -= cycles;
    }
    return 1; 
}

static void cycle_ppu(){
    switch(ppu.ppu_state){
        case OAM_Search:
            if(ppu.scanline_counter <= 456 - 80){
                ppu.ppu_state = Pixel_Transfer;
            }
        case Pixel_Transfer:
            //perform pixel fetching FFIO stuff
            switch(ppu.fetcher){
                case Fetch_Tile_NO:

                case Fetch_Tile_Low:

                case Fetch_Tile_High:

                case FFIO_Push:

                default: break;
            }

            //there is no register holding the current pixel so we must make it
            ppu.cur_pixel++;
            if(ppu.cur_pixel == 160){
                ppu.ppu_state = HBlank;
            }
        case HBlank:
            //when our scanline is 0 we move to next line
            //so we need to update LY and reset our counter
            //if we run out of vertical scanlines this indicates
            //we have reached vblank
            if(ppu.scanline_counter <= 0){
                ppu.scanline_counter = 456;
                mmu.memory[LY]++;
                if(read(LY) == 144){
                    ppu.ppu_state = VBlank;
                }else{
                    ppu.ppu_state = OAM_Search;
                }
            }
        case VBlank:
            //just need to check when we finish a line, update LY and counter
            //like before. If LY == 153 then we finished vblank and go back to 
            //OAM search
            if(ppu.scanline_counter <= 0){
                ppu.scanline_counter = 456; 
                mmu.memory[LY]++;
                if(read(LY) == 153){
                    write(LY, 0);
                    ppu.ppu_state = OAM_Search;
                }
            }
        default: break;
    }
}
