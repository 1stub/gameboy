#include "ppu.h"
#include "display.h"
#include "interrupt.h"

PPU ppu;
FETCHER fetcher;

static void cycle_ppu(int cycles);
static void 
update_fetcher(word tile_no_baseptr, word bg_tiledata_baseptr, byte is_signed, byte y_pos);
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
    ppu.is_window = 0;

    fetcher.state = Fetch_Tile_NO;
    fetcher.cycle_counter = 0;
    fetcher.cur_pixel = 0;
    fetcher.cur_tile_no = 0;
    fetcher.cur_tile_data_low = 0;
    fetcher.cur_tile_data_high = 0;
    fetcher.cur_tile_data_address = 0;
    fetcher.ticks = 0;
}

//when we transition to different stages of rendering a scanline we request an
//interrput specified by STAT
static void cycle_ppu(int cycles){
    int req_int = 0; 
    byte status = read(STAT);
    switch(ppu.ppu_state){
        case OAM_Search:
            //currently not dealing with sprites, so no OAM search
            if(ppu.scanline_counter <= 456 - 80){
                ppu.ppu_state = Pixel_Transfer;
            }
            break;
        case Pixel_Transfer:{
            word tile_no_baseptr = 0;
            const byte lcdc_status = read(LCDC);
            
            //window enable
            if(lcdc_status & (1 << 5)){
                if(read(WY) <= read(LY)){
                    ppu.is_window = 1;
                }else{
                    ppu.is_window = 0;
                }
            }else{
                ppu.is_window = 0;
            }

            if(ppu.is_window){
                //if using window, 6th bit gives window tile map
                if(lcdc_status & (1 << 6)){
                    tile_no_baseptr = 0x9C00;
                }else{
                    tile_no_baseptr = 0x9800;
                }
            }else{
                //otherwise we are using bg, so check 3rd bit
                if(lcdc_status & (1 << 3)){
                    tile_no_baseptr = 0x9C00;
                }else{
                    tile_no_baseptr = 0x9800;
                }
            }

            word bg_tiledata_baseptr = 0;
            byte is_signed = 0;
            if(lcdc_status & (1 << 4)){
                bg_tiledata_baseptr = 0x8000;
            }else{
                bg_tiledata_baseptr = 0x9000;
                is_signed = 1;
            }

            //now run checks to determine which of the 32 vertical tiles
            //our scanline is currently drawing, send to fetcher
            byte y_pos = 0;
            if(!ppu.is_window){
                y_pos = read(SCY) - read(LY);
            }else{
                y_pos = read(LY) - read(WY);
            }

            update_fetcher(tile_no_baseptr, bg_tiledata_baseptr, is_signed, y_pos);

            //160 pixels in a scanline
            if(fetcher.cur_pixel >= 160){
                if(status & (1<<3)){
                    req_int = 1;
                }
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
                    update_display_buffer(ppu.pixel_buffer);
                    if(status & (1<<4)){
                        req_int = 1;
                    }
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
                ppu.scanline_counter = 456;
                mmu.memory[LY]++;
                if(read(LY) == 153){ 
                    write(LY, 0);
                    if(status & (1<<5)){
                        req_int = 1;
                    }
                    ppu.ppu_state = OAM_Search;
                }
            }
            break;
        default: break;
    }
   
    //request lcd interrupt
    if(req_int){
        request_interrupt(1);
    }

    //check if ly == lyc, request interrupt
    if(read(LY) == read(LYC)){
        status &= (1 << 2);
        if(status & (1 << 6)){
            request_interrupt(1);       
        }
    }else{
        status &= ~(1 << 2); 
    }
    write(STAT, status);
}

static void 
update_fetcher(word tile_no_baseptr, word bg_tiledata_baseptr, byte is_signed, byte y_pos){
    fetcher.ticks++;
    if(fetcher.ticks < 2) return;
    else fetcher.ticks = 0;

    switch(fetcher.state){
        case Fetch_Tile_NO:{
            word row = (((byte)(y_pos/8) & 0xFF) * 32);
            word col = ((byte)(fetcher.cur_pixel/8));
            fetcher.cur_tile_no = read(tile_no_baseptr + row + col);
            fetcher.state = Fetch_Tile_Low;
            }break;
        case Fetch_Tile_Low:{
            byte line = (y_pos % 8) * 2;
            fetcher.cur_tile_data_address = fetcher.cur_tile_no + bg_tiledata_baseptr + line;
            if(is_signed){
                fetcher.cur_tile_data_low = (int8_t)read(fetcher.cur_tile_data_address);
            }else{
                fetcher.cur_tile_data_low = (int8_t)read(fetcher.cur_tile_data_address);
            }

            fetcher.state = Fetch_Tile_High;
            }break;
        case Fetch_Tile_High:
            if(is_signed){
                fetcher.cur_tile_data_high = (int8_t)read(fetcher.cur_tile_data_address + 1);
            }else{
                fetcher.cur_tile_data_high = (int8_t)read(fetcher.cur_tile_data_address + 1);
            }
            fetcher.state = FIFO_Push;
            break;
        case FIFO_Push:
            for(int i = 0; i < 8; i++){
                uint32_t color = get_color(
                    fetcher.cur_tile_data_low, fetcher.cur_tile_data_high, i);
                ppu.pixel_buffer[fetcher.cur_pixel + i][read(LY)] = color;
            }

            fetcher.cur_pixel += 8;
            fetcher.state = Fetch_Tile_NO;
            break;
        default: break;
    }
}

int update_graphics(int cycles){
    printf("LY = %i", read(LY));
    cycle_ppu(cycles);
    if(read(LCDC) & (1 << 7)){
        ppu.scanline_counter -= cycles;
    }else{
        //lcdc disabled, reset STAT and scanline
        ppu.scanline_counter = 456;
        write(LY, 0);
        byte status = read(STAT);
        status &= 252;
        write(STAT, status);
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
