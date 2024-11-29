#include "ppu.h"
#include "interrupt.h"

static void set_ppu_state();
static void update_lcd();

PPU ppu;

void ppu_init(){
    ppu.scanline_counter = 456;
}

void update_graphics(int cycles){ 
    update_lcd();
    if(read(LCDC) & (1<<7)){
        ppu.scanline_counter -= cycles;
    }else return ;

    //we hit 456 cycles, meaning we need to go to next line
    if(ppu.scanline_counter <= 0){
        byte cur_line = read(LY);
        mmu.memory[LY]++; //we dont want to reset LY here
        ppu.scanline_counter = 456;
        if(cur_line == 144){
            request_interrupt(0);
        }else if(cur_line > 153){
            write(LY, 0);
        }else{
            //draw the line
        }
    }
}

static void update_lcd(){
    set_ppu_state();
    switch(ppu.state){
        case OAM_Search:
        case Pixel_Transfer:
        case HBlank:
        case VBlank:
        default: break;
    }
}

static void set_ppu_state(){
    byte cur_line = read(LY);
    if(cur_line >= 144 && cur_line <= 153){
        ppu.state = VBlank;
    }
    if(cur_line > 153){
        ppu.state = OAM_Search;
    }
    else{
        int cur_cntr = ppu.scanline_counter;
        if(cur_cntr >= 456-80){
            ppu.state = OAM_Search;
        }else if(cur_cntr < 456 - 80 && cur_cntr >= 456 - 80 - 172){
            ppu.state = Pixel_Transfer;
        }else{
            ppu.state = HBlank;
        }
    }
}
