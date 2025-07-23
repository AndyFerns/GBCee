#include "rom.h"
#include "mmu.h"
#include <stdio.h>

extern uint8_t rom[0x8000]; // Access the ROM array defined in mmu.c

/**
 * load_rom - Loads a ROM file into memory starting at 0x0100.
 *
 * @path: Path to the ROM file.
 *
 * Return: 1 on success, 0 on failure.
 */
int load_rom(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("ROM open failed");
        return 0;
    }
    // Pad memory up to 0x0100 with NOPs (0x00)
    memset(rom, 0x00, 0x0100);

    // Load actual ROM starting at 0x0100
    size_t bytes_read = fread(&rom[0x0100], 1, 0x8000 - 0x0100, f);

    // fread(rom, 1, 0x8000, f); // Load up to 32KB
    fclose(f);
    return 1;
}
