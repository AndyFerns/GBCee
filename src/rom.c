#include "rom.h"
#include "mmu.h"
#include <stdio.h>

extern uint8_t rom[0x8000]; // Access the ROM array defined in mmu.c

/**
 * load_rom - See header.
 */
int load_rom(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("ROM open failed");
        return 0;
    }

    fread(rom, 1, 0x8000, f); // Load up to 32KB
    fclose(f);
    return 1;
}
