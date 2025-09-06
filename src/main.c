#include <stdio.h>
#include <stdbool.h> 
#include "cpu.h"
#include "mmu.h"
#include "timer.h"
#include "interrupts.h"

// TODO ppu.h, and timer.h


/**
 * @brief main:
 * Entry point of the emulator.
 * Loads the ROM and starts the emulation loop.
 *  
 * @param argc: Number of command-line arguments.
 * @param argv: Array of argument strings.
 *
 *
 * @returns 
 * 0 on success, non-zero on failure.
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }

    // should follow emulator lifecycle:
    // initialize hardware -> load the game -> run main loop -> clean up resources 

    // 1. Initialize hardware
    mmu_init();
    cpu_reset();
    // ppu_init();   // placeholder for initializing the Picture Processing Unit 
    // timer_init(); // placeholder for initializing the timer

    // 2. Load the game rom
    // only call mmu_load_rom and not load_rom
    if (mmu_load_rom(argv[1]) != 0) {
        fprintf(stderr, "Error: Failed to load ROM '%s'.\n", argv[1]);
        return 1;
    }

    // Main emulation loop
    printf(" --- Starting Emulation --- \n");
    while (true) { 
        /** Execute one instruction per cycle 
         * cpu step handles the halted state internally
         * doesnt fetch an opcode for halting
        */
        int cycles_this_step = cpu_step();

        // check if the cpu has halted and has 0 cycless this step
        if (cycles_this_step == 0) {
            break;
        }

        // update other hardware components with the elapsed cycles
        timer_step(cycles_this_step);
        // PLACEHOLDER: Future hardware steps will go here.
        // ppu_step(cycles_from_cpu);

        // Check for interrupts after all hardware has been updated
        handle_interrupts();
    }
    
    // 4. cleanup  
    printf(" --- Emulation Halted --- ");
    mmu_free(); // prevent memory leaks from loaded roms
    return 0;
}