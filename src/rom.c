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

    size_t bytes_read = fread(rom, 1, MAX_ROM_SIZE, f); // Full ROM load]
    fclose(f);

    if (bytes_read < 0x8000) {
        printf("Rom too small to be loaded!");
        return 0;
    }
    
    printf("Loaded %zu bytes at 0x0000\n", bytes_read);

    // detect MBC type from byte at 0x0147
    uint8_t mbc_type = rom[0x147];
    printf("MBC Type: 0x%02X\n", mbc_type);


    return 1;
}
