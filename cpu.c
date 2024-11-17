#include "cpu.h"

CPU cpu;

byte cycle(){
    execute(read(cpu.PC));
    cpu.A = 0x01;
    printf("A : %x\n", cpu.A);

    return 4;
}

void execute(byte opcode){
    
}
