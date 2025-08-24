// mbc.c
#include "mbc.h"
#include "mmu.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>


// removing all static and global variables

// // --- MBC1 state ---
// static uint8_t mbc1_rom_lo5 = 1;     // lower 5 bits of ROM bank (1..31)
// static uint8_t mbc1_rom_hi2_or_ram = 0; // high 2 bits for ROM (mode 0) or RAM bank (mode 1)
// static uint8_t mbc1_mode = 0;        // 0: ROM banking mode, 1: RAM banking mode

// // --- MBC2 state (scaffold) ---
// // MBC2 has internal 512x4-bit RAM, no external eram[]; enable/rom-bank writes depend on A8 bit.
// // We'll stub minimal behavior here.
// static uint8_t mbc2_rom_bank = 1;

// // --- MBC3 state (scaffold) ---
// // ROM bank: 7 bits (1..127), RAM bank 0..3 or RTC register (0x08..0x0C). Latch clock at 0x6000-0x7FFF.
// static uint8_t mbc3_rom_bank = 1;
// static uint8_t mbc3_ram_bank_or_rtc = 0;
// static uint8_t mbc3_latch = 0;

// // --- MBC5 state (scaffold) ---
// // ROM bank: 9 bits (0..511), RAM bank: 0..15
// static uint16_t mbc5_rom_bank = 1;
// static uint8_t  mbc5_ram_bank = 0;

// Helpers
static inline uint8_t in_rom_bounds(uint32_t off) {
    return (g_rom_size > 0 && off < g_rom_size);
}
static inline uint8_t in_eram_bounds(uint32_t off) {
    return (g_eram_size > 0 && off < g_eram_size);
}

/**
 * @brief initializes the memory bank controller system
 * 
 * @param type: uint8_t The MBC Type from the ROM header (0x0147)
 * 
 * @return void
 */ 
void mbc_init(uint8_t type) {
    mbc_type = type;
    ram_enable = 0;

    // Reset MBC1
    mbc1_rom_lo5 = 1;
    mbc1_rom_hi2_or_ram = 0;
    mbc1_mode = 0;

    // Reset others
    mbc2_rom_bank = 1;

    mbc3_rom_bank = 1;
    mbc3_ram_bank_or_rtc = 0;
    mbc3_latch = 0;

    mbc5_rom_bank = 1;
    mbc5_ram_bank = 0;
}

/**
 * @brief Compute active MBC1 ROM bank for 0x4000–0x7FFF window.
 * Rules:
 *  - Bank number = (hi2 << 5) | lo5
 *  - If lo5 == 0, treat as 1
 *  - In ROM mode (mode=0): lower window is 0x0000–0x3FFF (bank 0), upper window is selected bank.
 *  - In RAM mode (mode=1): same as above for upper window; lower window uses (hi2 << 5) with lo5=0.
 */
static inline uint16_t mbc1_active_upper_rom_bank(void) {
    uint8_t lo5 = mbc1_rom_lo5 & 0x1F;
    if (lo5 == 0) lo5 = 1; // forbidden to select 0 for upper bank
    uint8_t hi2 = (mbc1_rom_hi2_or_ram & 0x03);
    return (uint16_t)((hi2 << 5) | lo5);
}

/**
 * @brief Compute MBC1 lower-window bank (0x0000–0x3FFF) when in RAM banking mode.
 * In mode=1, "lower" window bank is (hi2 << 5) with low5 forced 0.
 */
static inline uint16_t mbc1_active_lower_rom_bank(void) {
    uint8_t hi2 = (mbc1_rom_hi2_or_ram & 0x03);
    return (uint16_t)(hi2 << 5);
}

/**
 * @brief Handles reads from 0x0000 to 0x7FFF ROM area
 * 
 * @param addr: uint16_t ROM Address
 * 
 * @return Byte read from banked ROM
 */
uint8_t mbc_read(uint16_t addr) {
    uint32_t off = 0;

    switch (mbc_type) {
    case MBC_TYPE_ROM_ONLY:
        // No banking; whole 0000–7FFF is just ROM[addr]
        off = (uint32_t)addr;
        return in_rom_bounds(off) ? rom[off] : 0xFF;

    case MBC_TYPE_MBC1: {
        if (addr < 0x4000) {
            // Lower window
            if (mbc1_mode == 0) {
                // ROM banking mode: bank 0
                off = (uint32_t)addr;
            } else {
                // RAM banking mode: bank = (hi2<<5), low5=0
                uint16_t bank = mbc1_active_lower_rom_bank();
                off = (uint32_t)bank * 0x4000u + (uint32_t)addr;
            }
        } else {
            // Upper window 0x4000–0x7FFF
            uint16_t bank = mbc1_active_upper_rom_bank();
            off = (uint32_t)bank * 0x4000u + (uint32_t)(addr - 0x4000);
        }
        return in_rom_bounds(off) ? rom[off] : 0xFF;
    }

    case MBC_TYPE_MBC2: {
        // 16 ROM banks max (4-bit bank), 0000–3FFF fixed, 4000–7FFF switchable
        uint16_t bank = (addr < 0x4000) ? 0 : (mbc2_rom_bank & 0x0F);
        if (bank == 0) bank = 1; // MBC2 also avoids bank 0 in upper window
        off = (uint32_t)bank * 0x4000u + (uint32_t)(addr & 0x3FFF);
        return in_rom_bounds(off) ? rom[off] : 0xFF;
    }

    case MBC_TYPE_MBC3: {
        // ROM bank 7 bits (1..127). Lower window is bank 0.
        if (addr < 0x4000) {
            off = (uint32_t)addr;
        } else {
            uint8_t bank = (mbc3_rom_bank & 0x7F);
            if (bank == 0) bank = 1;
            off = (uint32_t)bank * 0x4000u + (uint32_t)(addr - 0x4000);
        }
        return in_rom_bounds(off) ? rom[off] : 0xFF;
    }

    case MBC_TYPE_MBC5: {
        // ROM bank is 9 bits (0..511), lower window bank 0, upper window selected
        if (addr < 0x4000) {
            off = (uint32_t)addr;
        } else {
            uint16_t bank = mbc5_rom_bank; // 0..511 (0 is allowed on MBC5)
            off = (uint32_t)bank * 0x4000u + (uint32_t)(addr - 0x4000);
        }
        return in_rom_bounds(off) ? rom[off] : 0xFF;
    }

    default:
        // Fallback: behave like ROM_ONLY
        off = (uint32_t)addr;
        return in_rom_bounds(off) ? rom[off] : 0xFF;
    }
}

/**
 * @brief Handles write ops to 0x0000–0x7FFF (MBC control registers)
 * 
 * @param addr:uint16_t ROM address 
 * @param value:uint8_t value to be written
 * 
 * @return void 
 */
void mbc_write(uint16_t addr, uint8_t value) {
    switch (mbc_type) {
    case MBC_TYPE_ROM_ONLY:
        // No banking controls
        break;

    case MBC_TYPE_MBC1:
        if (addr < 0x2000) {
            // RAM enable: lower 4 bits must be 0xA
            ram_enable = ((value & 0x0F) == 0x0A);
        } else if (addr < 0x4000) {
            // ROM bank low 5 bits
            mbc1_rom_lo5 = (value & 0x1F);
            if ((mbc1_rom_lo5 & 0x1F) == 0) mbc1_rom_lo5 = 1; // avoid 0
        } else if (addr < 0x6000) {
            // ROM high 2 bits (in ROM mode) OR RAM bank (in RAM mode)
            mbc1_rom_hi2_or_ram = (value & 0x03);
        } else {
            // 0x6000–0x7FFF: mode select (0=ROM banking, 1=RAM banking)
            mbc1_mode = (value & 0x01);
        }
        break;

    case MBC_TYPE_MBC2:
        // Enable RAM only if A8==0 on write to 0000–1FFF
        if (addr < 0x2000) {
            if ((addr & 0x0100) == 0) {
                ram_enable = ((value & 0x0F) == 0x0A);
            }
        } else if (addr < 0x4000) {
            // ROM bank select only if A8==1
            if ((addr & 0x0100) != 0) {
                mbc2_rom_bank = (value & 0x0F);
                if (mbc2_rom_bank == 0) mbc2_rom_bank = 1;
            }
        }
        break;

    case MBC_TYPE_MBC3:
        if (addr < 0x2000) {
            ram_enable = ((value & 0x0F) == 0x0A);
        } else if (addr < 0x4000) {
            mbc3_rom_bank = (value & 0x7F);
            if (mbc3_rom_bank == 0) mbc3_rom_bank = 1;
        } else if (addr < 0x6000) {
            // RAM bank (0..3) or RTC reg (0x08..0x0C)
            mbc3_ram_bank_or_rtc = value;
        } else {
            // Latch clock (writes 0 then 1)
            // This is scaffolding; no RTC implemented yet.
            mbc3_latch = value;
        }
        break;

    case MBC_TYPE_MBC5:
        if (addr < 0x2000) {
            ram_enable = ((value & 0x0F) == 0x0A);
        } else if (addr < 0x3000) {
            // Low 8 bits of ROM bank
            mbc5_rom_bank = (mbc5_rom_bank & 0x100) | value;
        } else if (addr < 0x4000) {
            // High 1 bit of ROM bank
            mbc5_rom_bank = (mbc5_rom_bank & 0x0FF) | ((value & 0x01) << 8);
        } else if (addr < 0x6000) {
            // RAM bank 0..15
            mbc5_ram_bank = (value & 0x0F);
        }
        break;

    default:
        // Treat as ROM_ONLY
        break;
    }
}

/**
 * @brief Handles external RAM reads (0xA000–0xBFFF)
 * 
 * @param addr:uint16_t the address to read from
 * 
 * @returns unsigned integer 8_t
 */
uint8_t mbc_read_ram(uint16_t addr) {
    if (!ram_enable) return 0xFF;

    switch (mbc_type) {
    case MBC_TYPE_ROM_ONLY:
        // No external RAM specified; return open bus
        return 0xFF;

    case MBC_TYPE_MBC1: {
        // RAM bank depends on mode.
        // mode=0 -> RAM bank 0
        // mode=1 -> RAM bank = hi2 (0..3)
        uint8_t ram_bank = (mbc1_mode == 0) ? 0 : (mbc1_rom_hi2_or_ram & 0x03);
        uint32_t base = (uint32_t)ram_bank * 0x2000u; // 8KB per bank
        uint32_t off  = base + (uint32_t)(addr - 0xA000);
        return in_eram_bounds(off) ? eram[off] : 0xFF;
    }

    case MBC_TYPE_MBC2:
        // Uses internal 512x4-bit RAM, not our eram[]. For now, stub to 0xFF.
        return 0xFF;

    case MBC_TYPE_MBC3: {
        // If RAM bank 0..3 selected, access ERAM
        uint8_t sel = mbc3_ram_bank_or_rtc;
        if (sel <= 0x03) {
            uint32_t base = (uint32_t)sel * 0x2000u;
            uint32_t off  = base + (uint32_t)(addr - 0xA000);
            return in_eram_bounds(off) ? eram[off] : 0xFF;
        } else {
            // 0x08..0x0C -> RTC registers (not implemented yet)
            return 0xFF;
        }
    }

    case MBC_TYPE_MBC5: {
        uint32_t base = (uint32_t)(mbc5_ram_bank & 0x0F) * 0x2000u;
        uint32_t off  = base + (uint32_t)(addr - 0xA000);
        return in_eram_bounds(off) ? eram[off] : 0xFF;
    }

    default:
        return 0xFF;
    }
}

/**
 * @brief Handles external RAM writes (0xA000–0xBFFF)
 * 
 * @param addr:uint16_t The address to write to
 * @param value:uint8_t the value to be written
 * 
 * @returns void
 */
void mbc_write_ram(uint16_t addr, uint8_t value) {
    if (!ram_enable) return;

    switch (mbc_type) {
    case MBC_TYPE_ROM_ONLY:
        return;

    case MBC_TYPE_MBC1: {
        uint8_t ram_bank = (mbc1_mode == 0) ? 0 : (mbc1_rom_hi2_or_ram & 0x03);
        uint32_t base = (uint32_t)ram_bank * 0x2000u;
        uint32_t off  = base + (uint32_t)(addr - 0xA000);
        if (in_eram_bounds(off)) eram[off] = value;
        return;
    }

    case MBC_TYPE_MBC2:
        // 4-bit internal RAM; not implemented here.
        return;

    case MBC_TYPE_MBC3: {
        uint8_t sel = mbc3_ram_bank_or_rtc;
        if (sel <= 0x03) {
            uint32_t base = (uint32_t)sel * 0x2000u;
            uint32_t off  = base + (uint32_t)(addr - 0xA000);
            if (in_eram_bounds(off)) eram[off] = value;
        } else {
            // RTC registers (0x08..0x0C) — not implemented
        }
        return;
    }

    case MBC_TYPE_MBC5: {
        uint32_t base = (uint32_t)(mbc5_ram_bank & 0x0F) * 0x2000u;
        uint32_t off  = base + (uint32_t)(addr - 0xA000);
        if (in_eram_bounds(off)) eram[off] = value;
        return;
    }

    default:
        return;
    }
}
