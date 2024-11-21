#include "cpu.h"

CPU cpu;

static const byte cycles[0x100];
static const byte pc_inc[0x100];
static const byte extended_cycles[0x100];
static const byte extended_pc_inc[0x100];

#define SET_FLAGS(z,n,h,c) \
    if(z) F |= FLAG_Z; \
    if(n) F |= FLAG_N; \
    if(h) F |= FLAG_H; \
    if(c) F |= FLAG_C; 

//left variable is dst 
static inline void ADD(byte x, byte y) {
    word res = x + y; 
    SET_FLAGS((byte)res, 
            0, 
            (x & 0x0F) + (y & 0x0F) > 0x0F, 
            res > 0xFF); 
    x = x + y;
}

void cpu_init(){
    AF = 0x01B0;
    BC = 0x0013;
    DE = 0x00D8;
    HL = 0x014D;
    PC = 0x100;
    SP = 0xFFFE; 
    IME = 0;
}

//our cpu will cycle in increments of 4 T cycles (1 Machine Cycle) 
byte cycle(){
    byte cur_instr = read(PC);
    execute(cur_instr);
    
    PC += pc_inc[cur_instr];
    return cycles[cur_instr];
}

static void execute(byte opcode){
    switch(opcode){
        case 0x00: break;
        case 0x80: ADD(A,B); break;
        default: break;
    }
}

//sometimes cycles/pc increments can vary but this will cover most cases
static const byte cycles[0x100] = {
  /*0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F	*/
    4,12, 8, 8, 4, 4, 8, 4,20, 8, 8, 8, 4, 4, 8, 4,	/* 0x00 */
    4,12, 8, 8, 4, 4, 8, 4,12, 8, 8, 8, 4, 4, 8, 4,	/* 0x10 */
    8,12, 8, 8, 4, 4, 8, 4, 8, 8, 8, 8, 4, 4, 8, 4,	/* 0x20 */
    8,12, 8, 8,12,12,12, 4, 8, 8, 8, 8, 4, 4, 8, 4,	/* 0x30 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x40 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x50 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x60 */
    8, 8, 8, 8, 8, 8, 4, 8, 4, 4, 4, 4, 4, 4, 8, 4, /* 0x70 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x80 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0x90 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0xA0 */
    4, 4, 4, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4, 4, 8, 4,	/* 0xB0 */
    8,12,12,16,12,16, 8,16, 8,16,12, 8,12,24, 8,16,	/* 0xC0 */
    8,12,12, 0,12,16, 8,16, 8,16,12, 0,12, 0, 8,16,	/* 0xD0 */
    12,12,8, 0, 0,16, 8,16,16, 4,16, 0, 0, 0, 8,16,	/* 0xE0 */
    12,12,8, 4, 0,16, 8,16,12, 8,16, 4, 0, 0, 8,16	/* 0xF0 */
};

static const byte pc_inc[0x100] = {      
  /*0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F	*/
    1, 3, 1, 1, 1, 1, 2, 1, 3, 1, 1, 1, 1, 1, 1, 2, 1, /* 0x00 */
    2, 3
};

static const byte extended_cycles[0x100] = {
};

static const byte extended_pc_inc[0x100] = {
};
