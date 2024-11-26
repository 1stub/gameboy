#include "emulator.h"
#include "cpu.h"
#include "mmu.h"

static void do_interrupts();

void emulate(int debug){
    while(1){
        if(debug) print_registers();
        else{
            char out = perform_serial();
            if(out != '\0') printf("%c", out);
        }
        cycle(); 
        do_interrupts();
    }
}

void service_interrupt(int i){
    IME = 0;
    byte req = read(IF);
    req &= ~(1 << i); //clear interrupt bit
    write(IF, req);

    PUSH(&PC); //this is a direct call to push instruction from our cpu

    switch(i){ 
        case 0: PC = 0x40; break; //vblank
        case 1: PC = 0x48; break; //LCD
        case 2: PC = 0x50; break; //Timer
        case 3: PC = 0x58; break; //serial
        case 4: PC = 0x60; break; //Joypad
    }
}

static void do_interrupts(){
    if(IME){
        byte req = read(IF);
        byte flag = read(IE);
        if(req > 0){
            for(int i = 0; i < 5; i++){
                if(req & (1 << i)){
                    if(flag & (1 << i)){
                        service_interrupt(i);
                    }
                }
            }
        }
    }
}
