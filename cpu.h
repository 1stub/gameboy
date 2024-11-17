#ifndef CPU_H
#define CPU_H

#include <stdbool.h>
#include "util.h"
#include "regs.h"
#include "mmu.h"

#define FLAG_Z 1<<7
#define FLAG_N 1<<6
#define FLAG_H 1<<5
#define FLAG_C 1<<4

typedef struct{
    struct{
        union{
            struct {
                byte f;
                byte a;
            } ;
            word af;
        };
    };
    struct{
        union {
            struct {
                byte c;
                byte b;
            };
            word bc;
        };
    };
    struct{
        union {
            struct {
                byte e;
                byte d;
            };
            word de;
        };
    };
    struct{
        union {
            struct {
                byte l;
                byte h;
            };
            word hl;
        }; 
    };
} registers;

typedef struct{
    registers regs;
    byte IME;
    word SP;
    word PC;
} CPU;

#define A  regs.a
#define F  regs.f
#define AF regs.af
#define C  regs.c
#define B  regs.b
#define CB regs.cb
#define D  regs.d
#define E  regs.e
#define DE regs.de
#define H  regs.h
#define L  regs.l
#define HL regs.hl

byte cycle();
void execute(byte opcode);

#endif
