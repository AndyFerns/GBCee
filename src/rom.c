#include "rom.h"
#include "mmu.h"
#include <stdio.h>
#include <string.h> //for memset()

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
        fclosef(f);
        return 0;
    }

    // set file buffer
    uint8_t* buffer = malloc(size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for ROM.\n");
        fclosef(f);
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
        case 0x00: *out_mbc_type = MBC_TYPE_NONE; break;
        // TBD for all cartridge types

        default: *out_mbc_type = MBC_TYPE_UNKNOWN; break;
    }

    /* Setting output preferences */
    *out_rom_data = buffer;
    *out_rom_size = size;

    printf("Loaded %zu bytes from %s\n", size, path);

    return 1; //success
}
