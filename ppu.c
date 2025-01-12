#include "ppu.h"
#include "display.h"
#include "interrupt.h"

PPU ppu;

static void cycle_ppu(int cycles);
static void update_bg_scanline();
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position);
static TileOffsets calc_tile_offsets();
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
                ppu.cycles -= LCDC_MODE_OAM_CYCLES;
                update_bg_scanline();
                ppu.state = Pixel_Transfer;
                update_stat(HBLANK_NUM);
            }
            break;

        case HBlank:
            if(ppu.cycles >= LCDC_MODE_HBLANK_CYCLES){
                ppu.cycles -= LCDC_MODE_HBLANK_CYCLES;
                mmu.memory[LY] += 1;
                check_lyc_int();

                if(LY_VAL == 144){
                    request_interrupt(0);
                    update_stat(VBLANK_NUM);
                    ppu.state = VBlank;
                }else{
                    update_stat(OAM_NUM);
                    ppu.state = OAM_NUM;
                }
            }
            break;

        case VBlank:
            if(ppu.cycles >= 456){
                ppu.cycles -= 456;
                mmu.memory[LY] += 1;
                check_lyc_int();

                if(LY_VAL == 153){
                    ppu.should_update_display = 1;
                    write(LY, 0);
                    update_stat(OAM_NUM);
                }
            }
            break;

        default: break;
    }
}

//decided to just render whole scanline at once
static void update_bg_scanline(){
    TileOffsets offsets = calc_tile_offsets();
    int tile_data_bp = offsets.tile_data_bp;
    int tile_map_bp = offsets.tile_map_bp;
    int is_signed = offsets.is_signed;

    byte tile_data_low = 0x00;
    byte tile_data_high = 0x00;

    for(int i = 0; i < 160; i++){
        ppu.pixel_buffer[LY_VAL][i] = 
            get_color(tile_data_high, tile_data_low, i);
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
    write(STAT, (lyc == LY_VAL) << 6);

    if(lyc == LY_VAL && read(STAT) & (1 << 6)){
        request_interrupt(1);
    }
}

static TileOffsets 
calc_tile_offsets(){
    TileOffsets offsets;

    if (read(LCDC) & (1 << 4)) {
        offsets.tile_data_bp = 0x8000;
        offsets.is_signed = 0;
    } else {
        offsets.tile_data_bp = 0x9000;
        offsets.is_signed = 1;
    }

    if (read(LCDC) & (1 << 3)) {
        offsets.tile_map_bp = 0x9C00;
    } else {
        offsets.tile_map_bp = 0x9800;
    }

    return offsets;
}
