#include "emulator.h"

void emulate(int debug){
    static int running = 1;
    while(running){
        if(debug) print_registers();
        if(!debug){
            char out = perform_serial();
            if(out != '\0') printf("%c", out);
        }
        byte cycles = cycle(); 
        update_timers(cycles);
        running = update_graphics(cycles);
        do_interrupts();
    }
}
