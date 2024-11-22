#ifndef MMU_H
#define MMU_H

#include "util.h"
#include "regs.h"
#include <stdio.h>

typedef struct{
    byte memory[0xFFFF];
}MMU;

void mmu_init();
byte read(word address);
void write(word address, byte value);
void load_rom(char *file);
char perform_serial();

#endif
