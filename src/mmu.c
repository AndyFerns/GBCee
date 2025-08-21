// mmu.c
#include "mmu.h"
#include "mbc.h"            // NEW: delegate banking to MBC
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/// Max rom size supported (2MB)
#define ROM_FIXED_SIZE 0x8000

/// Max external RAM size (32KB) — matches mmu.h
// #define MAX_ERAM_SIZE (32 * 1024)  // now in mmu.h

/// VRAM size (8KB)
#define VRAM_SIZE 0x2000

/// WRAM size (8KB)
#define WRAM_SIZE 0x2000

/// HRAM size (127B)
#define HRAM_SIZE 0x7F

/// OAM size (160 bytes)
#define OAM_SIZE 0xA0

/// IO register size (128 bytes)
#define IO_SIZE 0x80

// Cartridge ROM region (not static — exposed in mmu.h)
uint8_t rom[MAX_ROM_SIZE];

/// VRAM: 8KB (0x8000–0x9FFF)
static uint8_t vram[VRAM_SIZE];

/// External RAM (cartridge): up to 32KB (0xA000–0xBFFF)
// MADE NON-STATIC so MBC can access it (declared extern in mmu.h)
uint8_t eram[MAX_ERAM_SIZE];

/// Work RAM (WRAM): 8KB (0xC000–0xDFFF)
static uint8_t wram[WRAM_SIZE];

/// High RAM (HRAM): 127B (0xFF80–0xFFFE)
static uint8_t hram[HRAM_SIZE];

/// Sprite Attribute Table (OAM): 160B (0xFE00–0xFE9F)
static uint8_t oam[OAM_SIZE];

/// IO Ports: 128B (0xFF00–0xFF7F)
static uint8_t io[IO_SIZE];

/// Interrupt Enable Register (0xFFFF)
static uint8_t interrupt_enable;

/// Interrupt Flag Register (0xFF0F)
static uint8_t interrupt_flag;

/// Actual loaded sizes (defaults to 0 until set)
size_t g_rom_size = 0;
size_t g_eram_size = 0;

/**
 * @brief Set the actual ROM size in bytes (call after loading ROM).
 * @param sz: size_t Total bytes in ROM buffer (<= MAX_ROM_SIZE).
 * @return void
 */
void mmu_set_rom_size(size_t sz) {
    if (sz > MAX_ROM_SIZE) sz = MAX_ROM_SIZE;
    g_rom_size = sz;
}

/**
 * @brief Set the actual ERAM size in bytes (from header).
 * @param sz: size_t Total bytes of external RAM (<= MAX_ERAM_SIZE).
 * @return void
 */
void mmu_set_eram_size(size_t sz) {
    if (sz > MAX_ERAM_SIZE) sz = MAX_ERAM_SIZE;
    g_eram_size = sz;
}

/**
 * @brief Initializes the MMU and clears memory regions.
 * 
 * @param none
 * 
 * @returns none 
 */
void init_mmu() {
    memset(vram, 0, sizeof(vram));
    memset(eram, 0, sizeof(eram));
    memset(wram, 0, sizeof(wram));
    memset(oam,  0, sizeof(oam));
    memset(hram, 0, sizeof(hram));
    memset(io,   0, sizeof(io));
    interrupt_enable = 0;
    interrupt_flag = 0;
}

/**
 * @brief Reads a byte from the full memory map.
 *
 * Handles memory bank redirection as per address range.
 *
 * @param addr Address to read from.
 * @return Value at that address.
 */
uint8_t mmu_read(uint16_t addr) {
    if (addr < ROM_FIXED_SIZE) {
        // 0x0000–0x7FFF (ROM area): delegate to MBC (handles bank 0 + switchable)
        return mbc_read(addr);

    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        return vram[addr - 0x8000];

    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        // External RAM is MBC-controlled (enable/banks)
        return mbc_read_ram(addr);

    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        return wram[addr - 0xC000];

    } else if (addr >= 0xE000 && addr <= 0xFDFF) {
        // Echo RAM (mirror of 0xC000–0xDDFF). Our WRAM buffer is 8KB, so this maps cleanly.
        return wram[addr - 0xE000];

    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return oam[addr - 0xFE00];

    } else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
        // Unusable range
        return 0xFF;

    } else if (addr == 0xFF0F) {
        return interrupt_flag;

    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        // NOTE: most of these need special handling later (joypad/timers/LCD/etc.)
        return io[addr - 0xFF00];

    } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        return hram[addr - 0xFF80];

    } else if (addr == 0xFFFF) {
        return interrupt_enable;
    }

    return 0xFF; // Unmapped memory
}

/**
 * @brief Writes a byte to the full memory map.
 *
 * Handles memory protection and mirroring.
 *
 * @param addr Address to write to.
 * @param value Byte to write.
 */
void mmu_write(uint16_t addr, uint8_t value) {
    if (addr < ROM_FIXED_SIZE) {
        // 0x0000–0x7FFF writes hit MBC control registers (bank switching / RAM enable / mode)
        mbc_write(addr, value);
        return;

    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        vram[addr - 0x8000] = value;

    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        // External RAM is MBC-controlled (enable/banks)
        mbc_write_ram(addr, value);

    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        wram[addr - 0xC000] = value;

    } else if (addr >= 0xE000 && addr <= 0xFDFF) {
        wram[addr - 0xE000] = value;

    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        oam[addr - 0xFE00] = value;

    } else if (addr >= 0xFEA0 && addr <= 0xFEFF) {
        // Unusable range: ignore writes
        return;

    } else if (addr == 0xFF0F) {
        interrupt_flag = value;

    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        // NOTE: many of these need special behavior (DIV reset on write, DMA, STAT/LCDC, etc.)
        io[addr - 0xFF00] = value;

    } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        hram[addr - 0xFF80] = value;

    } else if (addr == 0xFFFF) {
        interrupt_enable = value;
    }
}
