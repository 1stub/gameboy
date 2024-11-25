#include "emulator.h"
#include "cpu.h"
#include "mmu.h"

void emulate(){
    while(1){
        print_registers();
        cycle(); 
        char out = perform_serial();
        if(out != '\0') printf("%c", out);
    }
}
