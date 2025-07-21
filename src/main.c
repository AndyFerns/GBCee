#include <stdio.h>
#include "cpu.h"
#include "mmu.h"
#include "rom.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }

    init_mmu();
    load_rom(argv[1]);

    cpu_reset();

    while (1) {
        cpu_step();
        // TODO: Handle timers, interrupts, graphics, etc.
    }

    return 0;
}
