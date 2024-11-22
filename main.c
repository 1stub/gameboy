#include <stdio.h>
#include "emulator.h"

int main(int argc, char **argv){
    if(argc != 2) return 0;
    printf("%s\n", argv[1]);
    cpu_init();
    load_rom(argv[1]);
    emulate();
}
