#include "../includes/mmu.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/// Max rom size supported (2MB)
#define MAX_ROM_SIZE (2 * 1024 * 1024)

/// Fixed ROM window size (0x0000–0x7FFF, 32KB)
#define ROM_FIXED_SIZE 0x8000

/// Max external RAM size (32KB)
#define MAX_ERAM_SIZE (32 * 1024)

/// VRAM size (8KB)
#define VRAM_SIZE 0x2000

/// WRAM size (8KB)
#define WRAM_SIZE 0x2000

/// HRAM size (127B + IE)
#define HRAM_SIZE 0x7F

/// OAM size (160 bytes)
#define OAM_SIZE 0xA0

/// IO register size (128 bytes)
#define IO_SIZE 0x80


/// 32KB fixed ROM (0x0000–0x7FFF) split into:
/// - 0x0000–3FFF: fixed bank
/// - 0x4000–7FFF: switchable bank (MBC1/3/etc.)

// Cartridge ROM region
// DONT KEEP STATIC FOR THE LOVE OF ALL THATS GOOD
uint8_t rom[MAX_ROM_SIZE]; 

/// VRAM: 8KB (0x8000–0x9FFF)
static uint8_t vram[VRAM_SIZE];

/// External RAM (cartridge): 32KB (0xA000–0xBFFF)
static uint8_t eram[MAX_ERAM_SIZE];

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
        return rom[addr]; // TODO: handle banking later
        
    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        return vram[addr - 0x8000];

    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        return eram[addr - 0xA000];

    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        return wram[addr - 0xC000];
        
    } else if (addr >= 0xE000 && addr <= 0xFDFF) {
        // Echo RAM (mirror of 0xC000–0xDDFF)
        return wram[addr - 0xE000];

    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        return oam[addr - 0xFE00];

    } else if (addr == 0xFF0F) {
        return interrupt_flag;

    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
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
        // TODO: Handle MBC register writes for ROM banking
        return; // ROM is read-only
    } else if (addr >= 0x8000 && addr <= 0x9FFF) {
        vram[addr - 0x8000] = value;

    } else if (addr >= 0xA000 && addr <= 0xBFFF) {
        eram[addr - 0xA000] = value;

    } else if (addr >= 0xC000 && addr <= 0xDFFF) {
        wram[addr - 0xC000] = value;

    } else if (addr >= 0xE000 && addr <= 0xFDFF) {
        wram[addr - 0xE000] = value;

    } else if (addr >= 0xFE00 && addr <= 0xFE9F) {
        oam[addr - 0xFE00] = value;

    } else if (addr == 0xFF0F) {
        interrupt_flag = value;

    } else if (addr >= 0xFF00 && addr <= 0xFF7F) {
        io[addr - 0xFF00] = value;

    } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
        hram[addr - 0xFF80] = value;

    } else if (addr == 0xFFFF) {
        interrupt_enable = value;

    }
}