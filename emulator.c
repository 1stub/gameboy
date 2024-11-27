#include "emulator.h"

void emulate(int debug){
    while(1){
        if(debug) print_registers();
        else{
            char out = perform_serial();
            if(out != '\0') printf("%c", out);
        }
        byte cycles = cycle(); 
        do_interrupts();
        update_timers(cycles);
    }
}
