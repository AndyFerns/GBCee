#include "rom.h"
#include "mmu.h"
#include <stdio.h>
#include <string.h> //for memset()

extern uint8_t rom[MAX_ROM_SIZE]; // Access the ROM array defined in mmu.c

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
    //void *memset(void *_Dst, int _Val, size_t _Size)
    // memset(rom, 0x00, 0x0100); // not needed for commercial roms 

    // Load actual ROM starting at 0x0100

    size_t bytes_read = fread(rom, 1, 0x8000, f); // Full 32KB load

    // fread(rom, 1, 0x8000, f); // Load up to 32KB
    fclose(f);

    printf("Loaded %zu bytes at 0x0000\n", bytes_read);
    return 1;
}
