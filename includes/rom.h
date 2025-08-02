#ifndef ROM_H
#define ROM_H

/*
* Gameboy Rom Architecture understanding: 

* Address Range         Purpose
* 0x0000–0x00FF	        Optional BIOS/boot ROM      (intercepted by bootloader)
* 0x0100–0x014F	        Game Boy cartridge          header
* 0x0150+	            Start of game code          (entry point)
*/

typedef enum mbc_t {
    MBC_NONE,
    MBC1,
    MBC3,
    MBC5,
    MBC_UNKNOWN,
} mbc_t;

extern mbc_t mbc_type;

/**
 * load_rom - Loads a Game Boy ROM into memory.
 * 
 * Parameters: 
 * @path: Path to the ROM file.
 *
 * Return: 1 on success, 0 on failure.
 */
int load_rom(const char* path);

#endif