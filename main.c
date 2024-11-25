#include <stdio.h>
#include "emulator.h"

int main(int argc, char **argv){
    //if(argc != 2) return 0;
    load_rom(argv[1]);
    mmu_init();
    cpu_init();
    emulate();
}
