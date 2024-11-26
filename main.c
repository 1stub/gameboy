#include <stdio.h>
#include "emulator.h"

int main(int argc, char **argv){
    if(argc != 3){
        printf("Expected three arguments: ./gameboy *.gb 1/0\n");
        printf("If third arg 1 output register info, if 0 output serial\n");
        return 0;
    }
    int is_debug = argv[2][0] - '0';
    load_rom(argv[1]);
    mmu_init();
    cpu_init();
    emulate(is_debug);
}
