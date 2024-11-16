#ifndef CPU_H
#define CPU_H

#include "util.h"

struct cpu{
    bool ime;
};

typedef struct {
    union {
        struct {
            byte f;
            byte a;
        };
        word af;
    };
} reg_af;

typedef struct {
    union {
        struct {
            byte c;
            byte b;
        };
        word bc;
    };
} reg_bc;

typedef struct {
    union {
        struct {
            byte e;
            byte d;
        };
        word de;
    };
} reg_de;

typedef struct {
    union {
        struct {
            byte l;
            byte h;
        };
        word hl;
    };
} reg_hl;

#endif
