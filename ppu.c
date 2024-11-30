#include "ppu.h"

static void set_ppu_state();
static void update_lcd();
static void draw_scanline();
static void render_tiles();

PPU ppu;

void ppu_init(){
    ppu.scanline_counter = 456;

    //eventually would like to use custom palette
    byte palette = read(BGP);
    ppu.color_palette[0] = palette & (0x03);
    ppu.color_palette[1] = palette & (0x0C);
    ppu.color_palette[2] = palette & (0x30);
    ppu.color_palette[3] = palette & (0xC0);
}

int update_graphics(int cycles){ 
    update_lcd();
    if(read(LCDC) & (1<<7)){
        ppu.scanline_counter -= cycles;
    }else return 1;

    //we hit 456 cycles, meaning we need to go to next line
    if(ppu.scanline_counter <= 0){
        byte cur_line = read(LY);
        mmu.memory[LY]++; //we dont want to reset LY here
        ppu.scanline_counter = 456;
        if(cur_line == 144){
            request_interrupt(0); //vblank interrupt
        }else if(cur_line > 153){
            write(LY, 0);
        }else{
            draw_scanline(); 
            return render_display(); 
        }
    }
    return 1;
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

static void draw_scanline(){
    //just dealing with background for now, no sprites
    if(read(LCDC) & (0x01)){ //BG & Window enable bit
        render_tiles();
    }
}

static void render_tiles(){
    //first we need to figure out where in memory
    //to read bg data from
    word tile_data = 0;
    word bg_mem = 0;
    byte is_signed = 0;
    byte window_enable = 0;

    byte scroll_y = read(SCY);
    byte scroll_x = read(SCX);
    byte window_y = read(WY);
    byte window_x = read(WX);

    byte lcdc_data = read(LCDC);
    if(lcdc_data & (1 << 5)){ //window enable 
        if(scroll_y <= window_y){
            window_enable = 1;
        }
    }

    if(lcdc_data & (1 << 4)){ //BG and Window tile data area
        tile_data = 0x8000; 
    }else{ //this region of mem uses signed data
        tile_data = 0x8800;
        is_signed = 1;
    }
    
    //these two checks may seem redundant, but the area of mem for our
    //bg dependes on the window enable bit being set
    if(!window_enable){
        if(lcdc_data & (1 << 3)){
            bg_mem = 0x9C00;
        }else{
            bg_mem = 0x9800;
        }
    }else{
        if(lcdc_data & (1 << 6)){
            bg_mem = 0x9C00;
        }else{
            bg_mem = 0x9800;
        }
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
