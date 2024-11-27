#include "timer.h"

void update_timers(byte cycles){
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

