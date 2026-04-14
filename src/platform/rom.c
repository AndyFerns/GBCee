#include "rom.h"
#include "mmu.h"
#include <stdio.h>
#include <string.h> //for memset()
#include <stdlib.h>

// removed extern rom array to prevent external exposure to rom 

/**
 * @brief load_rom - Loads a ROM file into memory starting at 0x0100.
 *
 * @param path Path to the ROM file.
 *
 * @returns 1 on success, 0 on failure.
 */
int load_rom(const char* path, uint8_t** out_rom_data, size_t* out_rom_size, mbc_type_t* out_mbc_type) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("ROM open failed");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // checking the minimal size of a valid header
    if (size < 0x150) {
        fprintf(stderr, "ROM File is too small.\n");
        fclose(f);
        return 0;
    }

    // set file buffer
    uint8_t* buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for ROM.\n");
        fclose(f);
        return 0;
    }

    if (fread(buffer, 1, size, f) != size) {
        fprintf(stderr, "Failed to read ROM file.\n");
        fclose(f);
        free(buffer);
        return 0;
    }

    /* Parsing the file */
    uint8_t mbc_code = buffer[0x147];
    switch(mbc_code) {
        // allocating cartrigde type bytes from the rom header
        
        // No-MBC
        case 0x00: *out_mbc_type = MBC_TYPE_NONE; break;

        // --- MBC 1 ---
        case 0x01: // MBC1 
        case 0x02: // MBC1 + RAM
        case 0x03: // MBC1 + BATTERY
            *out_mbc_type = MBC_TYPE_MBC1;
            break;

        // --- MBC2 ---
        case 0x05: // MBC2
        case 0x06: // MBC2 + BATTERY
            // *out_mbc_type = MBC_TYPE_MBC2; // UNCOMMENT after mbc 2 implementation
            *out_mbc_type = MBC_TYPE_UNKNOWN;
            break;

        // --- MBC 3 --- 
        case 0x0F: // MBC3 + TIMER + BATTERY
        case 0x10: // MBC3 + TIMER + RAM + BATTERY
        case 0x11: // MBC3
        case 0x12: // MBC3 + RAM 
        case 0x13: // MBC3 + RAM + BATTERY
            *out_mbc_type = MBC_TYPE_MBC3;
            break;

        // --- MBC 5 --- 
        case 0x19: // MBC 5 
        case 0x1A: // MBC5 + RAM
        case 0x1B: // MBC5 + RAM + BATTERY
        case 0x1C: // MBC5 + RUMBLE
        case 0x1D: // MBC5 + RUMBLE + RAM
        case 0x1E: // MBC5 + RUMBLE + RAM + BATTERY
            *out_mbc_type = MBC_TYPE_MBC5;
            break;

        // --- uncommon MBC types ---
        case 0x08: // ROM + RAM
        case 0x09: // ROM + RAM + BATTERY
            *out_mbc_type = MBC_TYPE_NONE; // behaves like ROM_ONLY but has RAM lol
            break;

        default: *out_mbc_type = MBC_TYPE_UNKNOWN; break;
    }

    /* Setting output preferences */
    *out_rom_data = buffer;
    *out_rom_size = size;

    printf("Loaded %zu bytes from %s\n", size, path);

    return 1; //success
}
