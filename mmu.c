#include "mmu.h"
#include "cpu.h"

#define MEM mmu.memory 

static void do_dma(byte data);
MMU mmu;

extern void mmu_init(){
    MEM[0xFF00] = 0x1F;
    MEM[TIMA] = 0x00 ; 
    MEM[TMA]  = 0x00 ; 
    MEM[TAC]  = 0x00 ; 
    MEM[NR] = 0x80 ;
    MEM[0xFF11] = 0xBF ;
    MEM[0xFF12] = 0xF3 ;
    MEM[0xFF14] = 0xBF ;
    MEM[0xFF16] = 0x3F ;
    MEM[0xFF17] = 0x00 ;
    MEM[0xFF19] = 0xBF ;
    MEM[0xFF1A] = 0x7F ;
    MEM[0xFF1B] = 0xFF ;
    MEM[0xFF1C] = 0x9F ;
    MEM[0xFF1E] = 0xFF ;
    MEM[0xFF20] = 0xFF ;
    MEM[0xFF21] = 0x00 ;
    MEM[0xFF22] = 0x00 ;
    MEM[0xFF23] = 0xBF ;
    MEM[0xFF24] = 0x77 ;
    MEM[0xFF25] = 0xF3 ;
    MEM[0xFF26] = 0xF1 ;
    MEM[LCDC] = 0x91 ; 
    MEM[SCY]  = 0x00 ;
    MEM[SCX]  = 0x00 ;
    MEM[LYC]  = 0x00 ;
    MEM[BGP]  = 0xFC ;
    MEM[OBP0] = 0xFF ;
    MEM[OBP1] = 0xFF ;
    MEM[0xFF4A] = 0x00 ;
    MEM[0xFF4B] = 0x00 ;
    MEM[IE] = 0x00 ;
}

byte read(word address){
    return MEM[address];
}

word read16(word address){
    return MEM[address] | (MEM[address+1] << 8);
}

void write(word address, byte value){
    if(address == DIV){
        MEM[address] = 0x00;
        mmu.divider_counter = 0;
    }
    else if(address == LY){
        MEM[address] = 0x00;
    }
    else if(address >= 0x8000 && address <=0x9FFF){ 
        MEM[address] = value;
    }
    else if(address <= 0x7FFF){
        //do nothing, attempted to write to ROM
    }
    else if(address == 0xFF00){ //joypad, only bits 4 and 5 can be written to
        MEM[address] = value & 0x30;
    }
    else if(address == 0xFF46){
        do_dma(value);
    }
    else if(address == LCDC){
        MEM[LCDC] = value;
    }
    else if (address == 0xFFFF){
        if(value == 0) IME = 0;
        else IME = 1;

        MEM[0xFFFF] = value;
    }
    else if (address == STAT){
        //bit 0 and 1 are read only
        MEM[STAT] = (MEM[STAT] & 0x03) | (value & 0xFC);
    }
    else{
        MEM[address] = value;
    }
}

void load_rom(char *file){
    FILE *fp;
    fp = fopen(file, "rb");
    if(!fp){
        perror("fopen - unable to open argv[1]");
        return ;
    }

    size_t ret = fread(MEM, sizeof(byte), 0x10000, fp);
    if (ret == 0) {
        fprintf(stderr, "fread() failed or file is empty\n");
    }
    if(!ret){
        fprintf(stderr, "fread() failed: %zu\n", ret);
    }

    fclose(fp);
}

char perform_serial(){
    if(!(MEM[SC] & (1 << 7))){
        return '\0';    
    }
    MEM[SC] &= ~(1 << 7);

    const char data = (char)MEM[SB];
    return data;
}

static void do_dma(byte data){
    word address = data << 8;
    for (int i = 0 ; i < 0xA0; i++){
        write(0xFE00 + i, read(address + i));
    }
}
