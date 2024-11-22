#include "mmu.h"

#define MEM mmu.memory 

static MMU mmu;

extern void mmu_init(){

}

byte read(word address){
    return MEM[address];
}

void write(word address, byte value){
    MEM[address] = value;
}

void load_rom(char *file){
    FILE *fp;
    fp = fopen(file, "rb");
    if(!fp){
        perror("fopen - unable to open argv[2]");
        return ;
    }

    size_t ret = fread(MEM, sizeof(byte), 0xFFFF, fp);
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
