#include "emulator.h"
#include "cpu.h"
#include "mmu.h"

static void do_interrupts();
static void update_timers(byte cycles);

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

void request_interrupt(int interrupt){
    byte data = read(IF);
    data |= (1 << interrupt);
    write(IF, data);
}

static void update_timers(byte cycles){
    static int timer_counter = 0;

    mmu.divider_counter += cycles;
    if(mmu.divider_counter >= 256){
        mmu.divider_counter -= 256;
        mmu.memory[DIV]++;
    }

    byte TAC_value = read(TAC);
    if(TAC_value & (1 << 2)){
        timer_counter += cycles;
        int timer_thresh = 0;
        byte speed = TAC_value & 0x03;
        switch(speed){ //we read the speed at which our clock should update
            case 0x00: timer_thresh = 1024; break; //freq 4096
            case 0x01: timer_thresh = 16; break; //freq 262114
            case 0x02: timer_thresh = 64; break; //freq 65536
            case 0x03: timer_thresh = 256; break; //freq 16382
            default: break;
        }
        if(timer_counter > timer_thresh){
            timer_counter -= timer_thresh;
            if(read(TIMA) == 0xFF){ //timer counter overflow, need interrupt
                write(TIMA, read(TMA));
                request_interrupt(2);
            }else{
                write(TIMA, read(TIMA)+1);
            }
        }
    }
}

static void service_interrupt(int interrupt){
    IME = 0;
    byte req = read(IF);
    req &= ~(1 << interrupt); //clear interrupt bit
    write(IF, req);

    PUSH(&PC); //this is a direct call to push instruction from our cpu

    switch(interrupt){ 
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
