#include <stdio.h>
#include <stdbool.h> 
#include "cpu.h"
#include "mmu.h"
#include "timer.h"
#include "rom.h" // not required as main shouldnt know about the rom info

// TODO ppu.h, interrupts.h and timer.h


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

    mmu_init();
    cpu_reset();
    // ppu_init();   // placeholder for initializing the Picture Processing Unit 
    // timer_init(); // placeholder for initializing the timer

    // only call mmu_load_rom and not load_rom
    if (mmu_load_rom(argv[1]) != 0) {
        fprintf(stderr, "Error: Failed to load ROM '%s'.\n", argv[1]);
        return 1;
    }

    
    printf(" --- Starting Emulation --- \n");
    while (true) { 
        // if (cpu.PC >= 0x8000) {  // Prevent out-of-ROM execution
        //     printf("[HALT] PC out of ROM bounds: 0x%04X\n", cpu.PC);
        //     break;
        // }

        /** Execute one instruction per cycle 
         * cpu step handles the halted state internally
         * doesnt fetch an opcode for halting
        */
        int cycles_this_step = cpu_step();

        // check if the cpu has halted
        if (cycles_this_step == 0) {
            break;
        }

        timer_step(cycles_this_step);
        // if (!cpu_step()) {
        //     break;
        // }
        // PLACEHOLDER: Future hardware steps will go here.
        // ppu_step(cycles_from_cpu);
        // timer_step(cycles_from_cpu);
        // handle_interrupts();
    }
    
    printf(" --- Emulation Halted --- ");
    mmu_free(); // prevent memory leaks from loaded roms
    return 0;
}