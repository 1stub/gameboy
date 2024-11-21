#include <stdio.h>
#include "emulator.h"

int main(int argc, char **argv){
    cpu_init();
    emulate();
}
