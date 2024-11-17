#ifndef MMU_H
#define MMU_H

#include "util.h"

typedef struct{
    byte memory[0xFFFF];
}MMU;

byte read(word address);
void write(word address, byte value);

#endif
