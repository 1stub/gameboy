#include "ppu.h"
#include "display.h"
#include "interrupt.h"

//this code sucks
//sucks HARD as shit. some hot garbage I hate it
//when I find the time (after finals) I need to rewrite EVERTHING
//well maybe not everything (display using texture of pixels is fine)
//
//i now have the "general" understanding of what I need to do in the ppu
//so HOPEFULLY a ppu rewrite will end up getting some bg tiles on the display
//i'm sure there are going to be even more rewrites and reiterations
//but for now just a general rewrite with my "deeper" understanding of what
//the ppu actually does should put me in a good enough spot to get some real
//proper pixels on the screen
//
//the main problem that i am aware of is that i just compute offsets for tile
//data wrong. i am almost 100% sure of this. also issues likely arrise due to
//poor interrupt timing and overally lack of sync with the cpu. i think though
//there should still be some proper pixels being drawn even if this shit
//is not cycle accurate.

PPU ppu;
FETCHER fetcher;

static void cycle_ppu(int cycles);
static void 
update_fetcher();
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position);
static void calc_tile_offsets();

void ppu_init(){
    ppu.cycles = 0;

    //eventually would like to use custom palette
    byte palette = read(BGP);
    ppu.color_palette[0] = palette & (0x03);
    ppu.color_palette[1] = palette & (0x0C);
    ppu.color_palette[2] = palette & (0x30);
    ppu.color_palette[3] = palette & (0xC0);

    ppu.state = OAM_Search;
    ppu.update_display = 0;
    ppu.is_window = 0;

    fetcher.state = Fetch_Tile_NO;
    fetcher.is_signed = 0;
    fetcher.cur_pixel = 0;
    fetcher.tile_map_bp = 0;
    fetcher.tile_data_bp = 0;

    fetcher.cur_tile_x = 0;
    fetcher.cur_tile_y = 0;
    fetcher.tile_no = 0;
    fetcher.tile_low = 0;
    fetcher.tile_high = 0;
}

//we need to set flags upon ppu state transfer
//we will set necessary flags, raise STAT interrupt request
//then call handler
//
//also current issue is related to the fact that our ppu never reaches the
//ly == 153 condition causing us to never display anything
//
//TODO: add stat flags upon state transfers to call lcd interrupt
static void cycle_ppu(int cpu_cycles){
    int request_int = 0;

    switch(ppu.state){
        case OAM_Search:
            if(ppu.cycles >= 80){
                //prepare offsets for entering pixel transfer
                calc_tile_offsets();
                write(STAT, (read(STAT) | 0xFC) | 0x03);
                ppu.state = Pixel_Transfer;
            }
            break;

        case Pixel_Transfer:
            update_fetcher();
            if(fetcher.cur_pixel == 160){
                fetcher.cur_pixel = 0;
                write(STAT, (read(STAT) | 0xFC) | 0x00);
                ppu.state = HBlank;
            }
            break;

        case HBlank:
            if(ppu.cycles >= 456){
                ppu.cycles = 0;
                mmu.memory[LY]++;
                if(read(LY) == 144){
                    write(STAT, (read(STAT) | 0xFC) | 0x01);
                    ppu.state = VBlank;
                }else{
                    write(STAT, (read(STAT) | 0xFC) | 0x02);
                    ppu.state = OAM_Search;
                }
            }
            break;

        case VBlank:
            if(ppu.cycles >= 456){
                ppu.cycles = 0;
                mmu.memory[LY]++;
                if(read(LY) == 153){
                    //update display at end of frame
                    ppu.update_display = 1;
                    write(LY, 0);
                    ppu.cycles = 0; 
                    write(STAT, (read(STAT) | 0xFC) | 0x02);
                    ppu.state = OAM_Search;
                }
            }
            break;

        default: break;
    }

    if(request_int){
        request_interrupt(1); //stat interrupt
    }
}

static void update_fetcher(){
    switch(fetcher.state){
        case Fetch_Tile_NO:
            fetcher.state = Fetch_Tile_Low;
            break;
        case Fetch_Tile_Low:
            fetcher.state = Fetch_Tile_High;
            break;
        case Fetch_Tile_High:
            fetcher.state = FIFO_Push;
            break;
        case FIFO_Push:
            for(int i = 0; i < 8; i++){
                ppu.pixel_buffer[fetcher.cur_pixel + i][read(LY)] = 0xFF0000FF;
            }
            fetcher.cur_pixel += 8;
            fetcher.state = Fetch_Tile_NO;
            break;
        default: break;
    }
}

int update_graphics(int cpu_cycles){
    if(read(LCDC) & (1 << 7)){
        ppu.cycles += cpu_cycles;
        cycle_ppu(cpu_cycles);
    }else{
        //lcdc disabled, reset STAT and scanline
        ppu.cycles = 0;
        fetcher.cur_pixel = 0;
        ppu.state = OAM_Search;
        write(LY, 0);
    }
    
    if(ppu.update_display){
        ppu.update_display = 0;
        return render_display(); 
    }
    return 1; 
}

//this is used to get color data for our buffer to send to sdl texture
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position) { 
    int color_id = (tile_high & (1 >> (7 - bit_position))) |
        (tile_low & (1 >> (7 - bit_position))) << 1;
    //would be better if I use the pallet in memory, fine for now
    switch (color_id) {
        case 0: return 0xFFFFFFFF; // White
        case 1: return 0xAAAAAAFF; // Light gray
        case 2: return 0x555555FF; // Dark gray
        case 3: return 0x000000FF; // Black
        default: return 0xFFFFFFFF;
    }
}

static void calc_tile_offsets(){
    if(read(LCDC) & (1 << 4)){
        fetcher.tile_data_bp = 0x8000;
        fetcher.is_signed = 0;
    }else{
        fetcher.tile_data_bp = 0x8800;
        fetcher.is_signed = 1;
    }

    if(read(LCDC) & (1 << 3)){
        fetcher.tile_map_bp = 0x9C00;
    }else{
        fetcher.tile_map_bp = 0x9800;
    }
}
