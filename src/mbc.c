#include "mbc.h"
#include "mmu.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

static uint8_t mbc_type = 0;
static uint8_t rom_bank = 1;
static uint8_t ram_enable = 0;

/**
 * @brief initializes the memory bank controller system
 * 
 * @param type: uint8_t The MBC Type from the ROM header (0x0147)
 * 
 * @return void
 */ 
void mbc_init(uint8_t type) {
    mbc_type = type;
    rom_bank = 1;
    ram_enable = 0;
}


/**
 * @brief Handles reads from 0x0000 to 0x7FFF ROM area
 * 
 * @param addr: uint16_t ROM Address
 * 
 * @return Byte read from banked ROM
 */
uint8_t mbc_read(uint16_t addr) {
    if (addr < 0x4000) {
        return rom[addr];  // Bank 0 always
    } else if (addr >= 0x4000 && addr < 0x8000) {
        uint32_t offset = rom_bank * 0x4000 + (addr - 0x4000);
        return rom[offset % MAX_ROM_SIZE];
    }
    return 0xFF;
}


/**
 * @brief Handles write ops from 0x0000 to 0x7FFF ROM area (Bank switching)
 * 
 * @param addr:uint16_t ROM address 
 * @param val:uint8_t value to be written
 * 
 * @return void 
 */
void mbc_write(uint16_t addr, uint8_t value) {
    if (addr < 0x2000) {
        ram_enable = (value & 0x0F) == 0x0A;
    } else if (addr < 0x4000) {
        uint8_t bank = value & 0x1F;
        if (bank == 0) bank = 1;
        rom_bank = bank;
    }
    // TODO: handle RAM banking, mode switching (MBC1 extended)
}



uint8_t mbc_read_ram(uint16_t addr) {
    if (!ram_enable) return 0xFF;
    return eram[addr - 0xA000];
}

/**
 * @brief External RAM write.
 */
void mbc_write_ram(uint16_t addr, uint8_t value) {
    if (!ram_enable) return;
    eram[addr - 0xA000] = value;
}