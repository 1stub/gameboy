#include "ppu.h"
#include "display.h"

PPU ppu;
FETCHER fetcher;

static void cycle_ppu(int cycles);
static void 
update_fetcher(word tile_no_baseptr, word bg_tiledata_baseptr, byte is_signed);
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position);

void ppu_init(){
    ppu.scanline_counter = 456;

    //eventually would like to use custom palette
    byte palette = read(BGP);
    ppu.color_palette[0] = palette & (0x03);
    ppu.color_palette[1] = palette & (0x0C);
    ppu.color_palette[2] = palette & (0x30);
    ppu.color_palette[3] = palette & (0xC0);

    ppu.ppu_state = OAM_Search;
    ppu.update_display = 0;

    fetcher.state = Fetch_Tile_NO;
    fetcher.cycle_counter = 0;
    fetcher.cur_pixel = 0;
    fetcher.cur_tile_no = 0;
    fetcher.cur_tile_data_low = 0;
    fetcher.cur_tile_data_high = 0;
    fetcher.cur_tile_data_address = 0;
}

static void cycle_ppu(int cycles){
    switch(ppu.ppu_state){
        case OAM_Search:
            //currently not dealing with sprites, so no OAM search
            if(ppu.scanline_counter <= 456 - 80){
                ppu.ppu_state = Pixel_Transfer;
            }
            break;
        case Pixel_Transfer:{
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
            word tile_no_baseptr = 0;
            const byte lcdc_status = read(LCDC);
            if(lcdc_status & (1 << 3)){
                tile_no_baseptr = 0x9C00;
            }else{
                tile_no_baseptr = 0x9800;
            }

            word bg_tiledata_baseptr = 0;
            byte is_signed = 0;
            if(lcdc_status & (1 << 4)){
                bg_tiledata_baseptr = 0x8000;
            }else{
                bg_tiledata_baseptr = 0x8800;
                is_signed = 1;
            }

            update_fetcher(tile_no_baseptr, bg_tiledata_baseptr, is_signed);

            //160 pixels in a scanline
            if(fetcher.cur_pixel == 160){
                fetcher.cur_pixel = 0;
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

//as of now this fetcher ONLY fetches background tiles
//many of the offsets being used will change if deailing with window tiles
static void 
update_fetcher(word tile_no_baseptr, word bg_tiledata_baseptr, byte is_signed){
    switch(fetcher.state){
        case Fetch_Tile_NO:{
            word offset = (fetcher.cur_pixel / 8) + 
                (32 * (((read(LY) + read(SCY)) & 0xFF) / 8));
            fetcher.cur_tile_no = read(tile_no_baseptr + offset);
            fetcher.state = Fetch_Tile_Low;
            }break;
        case Fetch_Tile_Low:{
            word offset = (2 * ((read(LY) + read(SCY)) % 8));
            if(is_signed){
               offset += (int16_t)fetcher.cur_tile_no;
            }else{
               offset += fetcher.cur_tile_no; 
            }
            fetcher.cur_tile_data_address = bg_tiledata_baseptr + offset;
            fetcher.cur_tile_data_low = read(fetcher.cur_tile_data_address);
            fetcher.state = Fetch_Tile_High;
            }break;
        case Fetch_Tile_High:
            fetcher.cur_tile_data_high = read(fetcher.cur_tile_data_address+1);
            fetcher.state = FIFO_Push;
            break;
        case FIFO_Push:
            for(int i = 7; i >= 0; i--){
                uint32_t color = get_color(
                    fetcher.cur_tile_data_low, fetcher.cur_tile_data_high, i);
                ppu.pixel_buffer[fetcher.cur_pixel + i][read(LY)] = color;
            }
            fetcher.state = Fetch_Tile_NO;
            break;
        default: break;
    }
    fetcher.cur_pixel+=8;
}

int update_graphics(int cycles){
    cycle_ppu(cycles);
    ppu.scanline_counter -= cycles;
    
    //printf("%i\n", read(LY)); 
    
    if(ppu.update_display){
        ppu.update_display = 0;
        return render_display(); 
    }
    return 1; 
}

//this is used to get color data for our buffer to send to sdl texture
static uint32_t get_color(byte tile_high, byte tile_low, int bit_position) {
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
