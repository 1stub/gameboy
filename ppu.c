#include "ppu.h"

PPU ppu;

void update_graphics(){
}

void update_scanline(){
    switch(ppu.state){
        case OAM_Search:{
            ppu.state = Pixel_Transfer;
        }
        case Pixel_Transfer:
            ppu.state = HBlank;
        case HBlank:{
            if(read(LY) == 144){        
                ppu.state = VBlank;
            }else{
                ppu.state = OAM_Search;
            }
        }
        case VBlank:
            ppu.state = OAM_Search;
        default: break;
    }
}
