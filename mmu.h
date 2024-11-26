#ifndef MMU_H
#define MMU_H

#include "util.h"
#include "regs.h"
#include <stdio.h>

typedef struct{
    byte memory[0x10000];
    int divider_counter;
}MMU;

extern MMU mmu;

extern void mmu_init();
extern byte read(word address);
extern word read16(word address);
extern void write(word address, byte value);
extern void load_rom(char *file);
char perform_serial();

#endif
