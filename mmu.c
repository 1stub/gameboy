#include "mmu.h"

MMU mmu;

byte read(word address){
    return mmu.memory[address];
}

void write(word address, byte value){
    mmu.memory[address] = value;
}
