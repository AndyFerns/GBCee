#include "mbc.h"
#include "mmu.h"
#include "rom.h"

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


/**
 * @brief initializes the memory bank controller system
 * 
 * @param mmu: a pointer to the main mmu struct
 * 
 * @return void
 */ 
void mbc_init(mmu_t* mmu) {
    // The mbc_type is already set in mmu->mbc_type by the ROM loader.
    // We just need to initialize the banking registers.
    mmu->ram_enabled = false;
    mmu->current_rom_bank = 1;
    mmu->current_ram_bank = 0;
    mmu->mbc1_mode = 0;
}

uint8_t mbc_read_rom(mmu_t* mmu, uint16_t addr) {
    uint32_t rom_offset = 0;

     // --- FIX: Use a switch to handle different MBC types correctly. ---
    switch (mmu->mbc_type) {
        case MBC_TYPE_NONE:
            // For ROM_ONLY, the address is the offset. No banking.
            rom_offset = addr;
            break;

        case MBC_TYPE_MBC1:
            if (addr < 0x4000) {
                // Bank 00 is usually at 0x0000-0x3FFF.
                // In advanced mode, this can change, but for now, this is correct.
                rom_offset = addr;
            } else {
                // The switchable bank is at 0x4000-0x7FFF.
                rom_offset = (mmu->current_rom_bank * 0x4000) + (addr - 0x4000);
            }
            break;

        // TODO: Add cases for MBC3, MBC5, etc.
        default:
            // Default to ROM_ONLY behavior for unknown types.
            rom_offset = addr;
            break;
    }

    if (rom_offset < mmu->rom_size) {
        return mmu->rom_data[rom_offset];
    }
    return 0xFF; // Return 0xFF if out of bounds.
}


/**
 * @brief Handles write ops from 0x0000 to 0x7FFF ROM area (Bank switching)
 * 
 * @param mmu: pointer to the main mmu struct to modify its sttae
 * @param addr:uint16_t ROM address 
 * @param val:uint8_t value to be written
 * 
 * @return void 
 */
void mbc_write_rom(mmu_t* mmu, uint16_t addr, uint8_t value) {
    // This function now modifies the state inside the passed mmu struct
    switch (mmu->mbc_type) {
        case MBC_TYPE_MBC1:
            if (addr < 0x2000) { // RAM Enable
                mmu->ram_enabled = ((value & 0x0F) == 0x0A);
            } else if (addr < 0x4000) { // ROM Bank Number (lower 5 bits)
                uint8_t bank_lo = value & 0x1F;
                if (bank_lo == 0) bank_lo = 1; // Bank 0 is not selectable here
                mmu->current_rom_bank = (mmu->current_rom_bank & 0xE0) | bank_lo;
            } else if (addr < 0x6000) { // RAM Bank or Upper ROM Bank bits
                if (mmu->mbc1_mode == 0) { // ROM Mode
                    mmu->current_rom_bank = (mmu->current_rom_bank & 0x1F) | ((value & 0x03) << 5);
                } else { // RAM Mode
                    mmu->current_ram_bank = value & 0x03;
                }
            } else { // Banking Mode Select
                mmu->mbc1_mode = value & 0x01;
            }
            break;

        // TODO: Implement logic for MBC3, MBC5, etc.
        // They will follow the same pattern of modifying mmu->... state variables.
        case MBC_TYPE_MBC3: 
        case MBC_TYPE_MBC5:
            // Placeholder
            break;

        case MBC_TYPE_NONE:
        default:
            // Do nothing for ROM ONLY cartridges
            break;
    }
}



/**
 * @brief Handles reads  from external ram area (0xA000-0xBFFF)
 * 
 * @param mmu: pointer to the main mmu struct to modify its state
 * @param addr:uint16_t the address to read from
 * 
 * @returns the byte from the correctly calculated ram area
 */
uint8_t mbc_read_ram(mmu_t* mmu, uint16_t addr) {
    if (!mmu->ram_enabled) {
        return 0xFF; // Open bus behavior
    }

    // Calculate offset into ERAM based on the current RAM bank
    uint32_t ram_offset = (mmu->current_ram_bank * 0x2000) + (addr - 0xA000);

    // Note: A proper implementation would check the ERAM size from the header.
    // For now, we assume it fits within our MAX_ERAM_SIZE buffer.
    if (ram_offset < MAX_ERAM_SIZE) {
        return mmu->eram[ram_offset];
    }
    return 0xFF;
}


/**
 * @brief Handles external RAM writes(0xA000 to 0xBFFF)
 * 
 * @param mmu: pointer to the main mmu struct to modify its sttae
 * @param addr:uint16_t The address to write to
 * @param val:uint8_t the value to be written
 * 
 * @returns void
 */
void mbc_write_ram(mmu_t* mmu, uint16_t addr, uint8_t value) {
    if (!mmu->ram_enabled) {
        return;
    }

    uint32_t ram_offset = (mmu->current_ram_bank * 0x2000) + (addr - 0xA000);

    if (ram_offset < MAX_ERAM_SIZE) {
        mmu->eram[ram_offset] = value;
    }
}