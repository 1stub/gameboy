#include "mmu.h"

extern MMU mmu;

void mmu_init(){

}

byte read(word address){
    return mmu.memory[address];
}

void write(word address, byte value){
    mmu.memory[address] = value;
}

void load_rom(char *file){
    FILE *fp;
    fp = fopen(file, "rb");
    size_t ret;
    if(!fp){
        perror("fopen - unable to open argv[2]");
        return ;
    }

    ret = fread(mmu.memory, sizeof(byte), 0xFFFF, fp);
    if(!ret){
        fprintf(stderr, "fread() failed: %zu\n", ret);
    }

    fclose(fp);
}

char perform_serial(){
    char data = (char)mmu.memory[SB];
    mmu.memory[SC] |= ~(1 << 7);
    return data;
}
