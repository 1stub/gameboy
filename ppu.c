#include "ppu.h"
#include "display.h"
#include "interrupt.h"

PPU ppu;

static void cycle_ppu(int cycles);
static void update_scanlines();
static void update_bg_scanline();
static void update_window_scanline();
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
    if(lcdc_val & (1 << 5)){
        // render window
        update_bg_scanline(); //rhis should be window
        printf("rendering window!\n");
    }
    if(lcdc_val & (1 << 0)){
        update_bg_scanline();
    }
}

static void update_bg_scanline() {
    TileOffsets offsets = calc_tile_offsets();
    int tile_data_bp = offsets.tile_data_bp;
    int tile_map_bp = offsets.tile_map_bp;
    int is_signed = offsets.is_signed;

    byte scanline = LY_VAL;

    byte scroll_x = read(SCX);
    byte scroll_y = read(SCY);

    for (int x = 0; x < 160; x++) {
        int map_x = (scroll_x + x) & 0xFF;
        int map_y = (scroll_y + scanline) & 0xFF;

        word tile_row = (map_y / 8) * 32; // Each row contains 32 tiles
        word tile_col = map_x / 8;

        word tile_map_addr = tile_map_bp + tile_row + tile_col;
        byte tile_id = read(tile_map_addr);

        word tile_data_offset;
        if (is_signed) {
            int8_t signed_id = (int8_t)tile_id;
            tile_data_offset = tile_data_bp + (signed_id * 16);
        } else {
            tile_data_offset = tile_data_bp + (tile_id * 16);
        }

        int tile_x_pixel = 7 - (map_x % 8); // Tiles are stored with pixels mirrored in X
        int tile_y_pixel = map_y % 8;

        word pixel_offset = tile_data_offset + (tile_y_pixel * 2);
        byte tile_data_low = read(pixel_offset);
        byte tile_data_high = read(pixel_offset + 1);

        ppu.pixel_buffer[scanline][x] = get_color(tile_data_high, tile_data_low, tile_x_pixel);
    }
}

static void update_window_scanline(){

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
    static int i = 0;

    if (read(LCDC) & (1 << 4)) {
        offsets.tile_data_bp = 0x8000;
        offsets.is_signed = 0;
    } else {
        offsets.tile_data_bp = 0x9000;
        offsets.is_signed = 1;
    }

    if(read(LCDC) & (1 << 5)){
        if (read(LCDC) & (1 << 6)) {
            offsets.tile_map_bp = 0x9C00;
        } else {
            offsets.tile_map_bp = 0x9800;
        }
    }else{
        if (read(LCDC) & (1 << 3)) {
            offsets.tile_map_bp = 0x9C00;
        } else {
            offsets.tile_map_bp = 0x9800;
        }
    }

    if(!i){
        printf("DATA: %x, MAP: %x\n", offsets.tile_data_bp, offsets.tile_map_bp);
    }
    i++;

    return offsets;
}
