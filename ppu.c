#include "ppu.h"
#include "display.h"
#include "interrupt.h"

PPU ppu;

static void cycle_ppu(int cycles);
static void update_scanlines();
static void update_bg_win_scanline();
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position);
static void check_lyc_int();
static void update_stat(int state);

void ppu_init(){
    ppu.cycles = 0;
    ppu.state = OAM_Search;
    ppu.should_update_display = 0;
    ppu.is_window = 0;
}

static void cycle_ppu(int cpu_cycles){
    if (!(read(LCDC) & (1 << 7))) { // LCDC enable
        if(ppu.cycles >= 70224){
            ppu.state = OAM_Search;
            ppu.cycles -= 70224;
        }
        return ;
    }

    ppu.cycles += cpu_cycles;

    switch(ppu.state){
        case OAM_Search:
            if(ppu.cycles >= LCDC_MODE_OAM_CYCLES){
                ppu.cycles -= LCDC_MODE_OAM_CYCLES;
                update_stat(PIXEL_TRANSFER_NUM);
                ppu.state = Pixel_Transfer;
            }
            break;

        case Pixel_Transfer:
            if(ppu.cycles >= LCDC_MODE_PIXEL_CYCLES){
                ppu.cycles -= LCDC_MODE_PIXEL_CYCLES;
                update_scanlines();
                update_stat(HBLANK_NUM);
                ppu.state = HBlank;
            }
            break;

        case HBlank:
            if(ppu.cycles >= LCDC_MODE_HBLANK_CYCLES){
                ppu.cycles -= LCDC_MODE_HBLANK_CYCLES;
                mmu.memory[LY]++;
                check_lyc_int();

                if(LY_VAL == 144){
                    request_interrupt(0);
                    update_stat(VBLANK_NUM);
                    ppu.state = VBlank;
                }else{
                    update_stat(OAM_NUM);
                    ppu.state = OAM_Search;
                }
            }
            break;

        case VBlank:
            if(ppu.cycles >= LCDC_LINE_CYCLES){
                ppu.cycles -= LCDC_LINE_CYCLES;
                mmu.memory[LY]++;
                check_lyc_int();

                if(LY_VAL == 153){
                    ppu.cycles = 0;
                    ppu.should_update_display = 1;
                    write(LY, 0);
                    update_stat(OAM_NUM);
                    ppu.state = OAM_Search;
                }
            }
            break;

        default: break;
    }
}

static void update_scanlines(){
    byte lcdc_val = read(LCDC);
    if(lcdc_val & 0x01){
        update_bg_win_scanline();
    }
}

static void update_bg_win_scanline() {
    byte lcdc_status = read(LCDC);
    byte scanline = LY_VAL;
    byte scroll_x = read(SCX);
    byte scroll_y = read(SCY);
    byte window_x = read(WX);
    byte window_y = read(WY);
    byte y_pos, x_pos;
    word tile_map_bp, tile_data_bp;
    byte is_signed, is_window;

    if((lcdc_status & (1<<5)) && (window_y <= scanline)){ //window
        y_pos = scanline - window_y;

        if(lcdc_status & (1<<6)){
            tile_map_bp = 0x9C00;
        }else{
            tile_map_bp = 0x9800;
        }
        is_window = 1;
    }else{ //no window
        y_pos = scroll_y + scanline;

        if(lcdc_status & (1<<3)){
            tile_map_bp = 0x9C00;
        }else{
            tile_map_bp = 0x9800;
        }
        is_window = 0;
    }

    if(lcdc_status & (1<<4)){
        tile_data_bp = 0x8000;
        is_signed = 0;
    }else{
        tile_data_bp = 0x8800;
        is_signed = 1;
    }

    for (int x = 0; x < 160; x++) {
        x_pos = (x + scroll_x) & 0xFF;

        if(is_window){ //enables wrap around effect
            byte reverse_wx = window_x - 7;
            if(x >= reverse_wx){
                x_pos = x - reverse_wx;
            }
        }

        word col_pos = x_pos / 8;
        word row_pos = ((y_pos/8))*32;
        word tile_address = tile_map_bp + ((row_pos + col_pos) & 0x3FF);

        if(is_signed){
            byte tile_number = read(tile_address);
            tile_address = tile_data_bp + ((tile_number + 128) * 16);
        }else{
            byte tile_number = read(tile_address);
            tile_address = tile_data_bp + (tile_number * 16);
        }

        byte line = (y_pos % 8) * 2;

        byte tile_data_low = read(tile_address + line);
        byte tile_data_high = read(tile_address + line + 1);

        ppu.pixel_buffer[scanline][x] = get_color(tile_data_high, tile_data_low, 7-(x % 8));
    }
}

int update_graphics(int cpu_cycles){
    cycle_ppu(cpu_cycles);

    if(ppu.should_update_display){
        ppu.should_update_display = 0;
        update_display_buffer(ppu.pixel_buffer);
        return 1; 
    }
    return 0; 
}

static void update_stat(int state){
    //update lowest two bits to update current state
    byte stat = read(STAT);
    stat = (stat & 0xFC) | state;
    write(STAT, stat);

    //now we check stat flags and see if we need to req lcd interrupt
    //we add 3 to our state since the corresponding bit in STAT (the flag)
    //is always 3 bits more than the actual state identifier
    DO_STAT_INTERRUPTS((state + 0x03));
}

//this is used to get color data for our buffer to send to sdl texture
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position) { 
    int color_id = ((tile_high >> bit_position) & 0x01) |
        ((tile_low >> bit_position) & 0x01) << 1;
    //would be better if I use the pallet in memory, fine for now
    switch (color_id) {
        case 0: return 0xFFFFFFFF; // White
        case 1: return 0xAAAAAAFF; // Light gray
        case 2: return 0x555555FF; // Dark gray
        case 3: return 0x000000FF; // Black
        default: return 0xFFFFFFFF;
    }
}

static void check_lyc_int(){
    byte lyc = read(LYC);

    write(STAT, (LY_VAL == lyc) << 6);

    if(lyc == LY_VAL && read(STAT) & (1 << 6)){
        request_interrupt(1);
    }
}
