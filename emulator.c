#include "emulator.h"
#include "display.h"

#include <cjson/cJSON.h>
#include <dirent.h>

#define CPU_FREQ 4194304
#define FPS 60
#define FRAME_HISTORY_SIZE 10

static void run_json_tests();

//we need to ensure that our emulator is running at roughly 60 fps,
//so mych of this code is output just to ensure we are close to
//~16.5 ms per frame render.

void emulate(int debug){
    static int total_cycles = 0;
    static int run = 1;
    static Uint64 prev_frame = 0;
    static double frame_times[FRAME_HISTORY_SIZE] = {0};
    static int frame_time_index = 0;
    Uint64 freq = SDL_GetPerformanceFrequency();

    while(run){
        Uint64 this_frame = SDL_GetPerformanceCounter();
        
        if(debug) ; //run_json_tests();

        byte cycles = cycle(); 
        update_timers(cycles);
        int can_render_frame = update_graphics(cycles);
        total_cycles += cycles;
        do_interrupts();
    
        if(can_render_frame && total_cycles >= (CPU_FREQ / FPS)){
            // Calculate frame time using high-resolution timer
            double frame_diff = (double)(this_frame - prev_frame) * 1000.0 / freq;
            frame_times[frame_time_index] = frame_diff;
            frame_time_index = (frame_time_index + 1) % FRAME_HISTORY_SIZE;

            // Calculate average frame time
            double avg_frame_time = 0.0;
            for (int i = 0; i < FRAME_HISTORY_SIZE; ++i) {
                avg_frame_time += frame_times[i];
            }
            avg_frame_time /= FRAME_HISTORY_SIZE;

            //printf("frame diff = %.3f ms, avg frame time = %.3f ms\n", frame_diff, avg_frame_time);

            prev_frame = this_frame;

            run = render_display(); 
        }
    }
}

//currently decided to go back and make sure we are passing BOTH json cpu
//tests and blargg tests. Could be part of our ppu problems.

//following methods are used if we want to test cpu json tests
//https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
//https://www.geeksforgeeks.org/cjson-json-file-write-read-modify-in-c/

//this is broke still
/*static void run_json_tests(){
    struct dirent *de;
    DIR *dr = opendir("./sm83/v1/");
    if(dr == NULL){
        printf("Unable to open directory");
        return;
    }

    while((de = readdir(dr)) != NULL){
        char loc[256];  
        snprintf(loc, sizeof(loc), "./sm83/v1/%s", de->d_name);

        FILE *fp = fopen(loc, "r");
        if(fp == NULL){
            printf("Unable to open json file");
        }
        printf("%s\n", de->d_name);
    }
    closedir(dr);
}*/
