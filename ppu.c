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
    ppu.update_display = 0;
}

int update_graphics(int cycles){
    cycle_ppu();
    ppu.scanline_counter -= cycles;
    
    //printf("%i\n", read(LY)); 
    
    if(ppu.update_display){
        ppu.update_display = 0;
        return render_display(); 
    }
    return 1; 
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
           
            word bg_mem_base = 0;

            //first find what area of bg memory we are accessing
            if(read(LCDC) & (1<< 3)){
                bg_mem_base = 0x9C00;
            }else{
                bg_mem_base = 0x9800;
            }

            //to get this to work properly here is what is need to be done:
            //(i think). First we need to use the THIRD bit of LCDC
            //to find the region we are currently reading background
            //map data from. This contains the TILE NUMBERS NOT DATA.
            //then we use the FOURTH bit of LCDC to find the region 
            //of memory that actually contains our tile data, not just the 
            //number. From here we can parse our data similarly to below
            //and send it to sdl. Also, the region that stores the actual
            //data is where we need to be careful about whether we read 
            //it as signed or unsigned.
            //0x8000 as base ptr uses UNSIGNED 
            //0x8800 as base ptr uses SIGNED

            //need to move all this ffio stuff into its own struct
            word tile_map_row_addr = bg_mem_base +
                (32 * (((read(LY) + read(SCY)) & 0xFF) / 8));
            byte tile_low = 0;
            byte tile_high = 0;
            byte tile_id = 0;
            static byte tile_index = 0; //the tile we are currently rendering

            word tile_address = (2 * (read(LY) + read(SCY)) % 8);

            switch(ppu.fetcher){
                case Fetch_Tile_NO:
                    ppu.fetcher = Fetch_Tile_Low;
                    tile_id = read(tile_map_row_addr + tile_index);
                    break;
                case Fetch_Tile_Low:
                    tile_low = read(tile_id + tile_address);
                    ppu.fetcher = Fetch_Tile_High;
                    break;
                case Fetch_Tile_High:
                    tile_high = read(tile_id + tile_address + 1);
                    ppu.fetcher = FIFO_Push;
                    break;
                case FIFO_Push:
                    //we use a 2d array that gets sent to our sdl window
                    for(int i = 7; i >= 0; i--){
                        uint32_t color = get_color(tile_high, tile_low, i);
                        ppu.pixel_buffer[ppu.cur_pixel + i][read(LY)] = color; 
                    }
                    tile_index++;
                    ppu.cur_pixel+=8;
                    ppu.fetcher = Fetch_Tile_NO;
                    break;
                default: break;
            }

            if(ppu.cur_pixel == 160){
                ppu.cur_pixel = 0;
                tile_index = 0;
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
                ppu.update_display = 1;
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
