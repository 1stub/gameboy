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
        do_interrupts();
        running = update_graphics(cycles);
        update_timers(cycles);
    }
}
