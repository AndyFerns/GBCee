#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "rom.h"

/**
 * main:
 * Entry point of the emulator.
 * 
 * Arguments: 
 * @argc: Number of command-line arguments.
 * @argv: Array of argument strings.
 *
 * Loads the ROM and starts the emulation loop.
 *
 * Return: 
 * 0 on success, non-zero on failure.
 */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }

    init_mmu();

    if (!load_rom(argv[1])) {
        printf("Failed to load ROM.\n");
        return 1;
    }

    cpu_reset();

    while (1) {
        // Execute one instruction per cycle
        cpu_step();

        // Future work:
        // - Handle timers
        // - Render graphics
        // - Handle input
        // - Trigger interrupts
    }

    return 0;
}