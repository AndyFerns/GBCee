#ifndef ROM_H
#define ROM_H

#include <stdint.h>
#include <stddef.h>

/*
* Gameboy Rom Architecture understanding: 

* Address Range         Purpose
* 0x0000–0x00FF	        Optional BIOS/boot ROM      (intercepted by bootloader)
* 0x0100–0x014F	        Game Boy cartridge          header
* 0x0150+	            Start of game code          (entry point)
*/

/** @brief Enum for MBC allocation */
typedef enum mbc_type_t {
    MBC_TYPE_NONE,
    MBC_TYPE_MBC1,
    MBC_TYPE_MBC2,
    MBC_TYPE_MBC3,
    MBC_TYPE_MBC5,
    MBC_TYPE_UNKNOWN,
} mbc_type_t;

extern mbc_type_t mbc_type;

/**
 * @brief load_rom - loads a ROM file, allocates memory for it, and parses its header
 * 
 * Parameters: 
 * @param path Path to the ROM file.
 * @param out_rom_data pointer to an uint8_t* that will be set to the allocated rom data
 * @param out_rom_size pointer to a size_t that will be set to the size of the rom
 * @param out_mbc_type pointer to an mbc_type_t
 *
 * @returns 1 on success, 0 on failure.
 */
int load_rom(const char* path, uint8_t** out_rom_data, size_t* out_rom_size, mbc_type_t* out_mbc_type);

#endif