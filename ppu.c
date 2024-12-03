#include "ppu.h"
#include "display.h"

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
    ppu.scanline_counter -= cycles;

    return render_display(); 
}

//this is used to get color data for our buffer to send to sdl texture
uint32_t get_color(byte tile_high, byte tile_low, int bit_position) {
    int color_id = (tile_high & (1<<bit_position)) | (tile_low << bit_position ) << 1;
    //would be better if I use the pallet in memory, fine for now
    switch (color_id) {
        case 0: return 0xFFFFFFFF; // White
        case 1: return 0xAAAAAAFF; // Light gray
        case 2: return 0x555555FF; // Dark gray
        case 3: return 0x000000FF; // Black
        default: return 0xFFFFFFFF;
    }
}

static void cycle_ppu(){
    switch(ppu.ppu_state){
        case OAM_Search:
            if(ppu.scanline_counter <= 456 - 80){
                ppu.ppu_state = Pixel_Transfer;
            }
            break;
        case Pixel_Transfer:{
            //perform pixel fetching FIFO stuff
            printf("%i\n", read(LY)); 
            //the folloiwng code doesnt work, just using it for testing
            byte tile_line = read(LY) % 2;
            word tile_map_row_addr = 0x9C00 + (32*(read(LY) / 8));
            byte tile_low = 0;
            byte tile_high = 0;

            switch(ppu.fetcher){
                case Fetch_Tile_NO:
                    ppu.fetcher = Fetch_Tile_Low;
                    break;
                case Fetch_Tile_Low:
                    tile_low = read(tile_map_row_addr + tile_line);
                    ppu.fetcher = Fetch_Tile_High;
                    break;
                case Fetch_Tile_High:
                    tile_high = read(tile_map_row_addr + tile_line + 1);
                    ppu.fetcher = FIFO_Push;
                    break;
                case FIFO_Push:
                    //we use a 2d array that gets sent to our sdl window
                    for(int i = 7; i >= 0; i--){
                        uint32_t color = get_color(tile_high, tile_low, i);
                        ppu.pixel_buffer[ppu.cur_pixel + i][read(LY)] = color; 
                    }
                    ppu.cur_pixel+=8;
                    ppu.fetcher = Fetch_Tile_NO;
                    break;
                default: break;
            }

            if(ppu.cur_pixel == 160){
                ppu.cur_pixel = 0;
                ppu.ppu_state = HBlank;
            }
            break;
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
            break;
        case VBlank:
            //just need to check when we finish a line, update LY and counter
            //like before. If LY == 153 then we finished vblank and go back to 
            //OAM search
            if(ppu.scanline_counter <= 0){
                update_display_buffer(ppu.pixel_buffer);
                ppu.scanline_counter = 456;
                mmu.memory[LY]++;
                if(read(LY) == 153){
                    write(LY, 0);
                    ppu.ppu_state = OAM_Search;
                }
            }
            break;
        default: break;
    }
}
