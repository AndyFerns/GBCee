#include "mmu.h"
#include "mbc.h"            // NEW: delegate banking to MBC
#include "rom.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// =================================================
// Global Size definitions
// =================================================

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

/// Max External RAM size (32KB)
#define MAX_ERAM_SIZE (32 * 1024)


// ===================================================
// MMU State Structure
// ===================================================

/// the core mmu struct which stores all memory and state
typedef struct mmu_t {
    // dynamically allocated rom data
    uint8_t* rom_data;
    size_t rom_size;

    // internal memory regions
    uint8_t vram[VRAM_SIZE];
    uint8_t eram[MAX_ERAM_SIZE];
    uint8_t wram[WRAM_SIZE];
    uint8_t oam[OAM_SIZE];
    uint8_t io[IO_SIZE];
    uint8_t hram[HRAM_SIZE];

    // internal registers
    uint8_t interrupt_enable;
    uint8_t interrupt_flag;

    // MBC (Memory bank controller) state
    mbc_type_t mbc_type;
    // other mbc types TBD
    // int current_rom_bank;
    // bool eram_enabled;
} mmu_t;

static mmu_t mmu;


// =========================================================
// Function Implementations
// ============================================================


/**
 * @brief Initializes the MMU memory regions
 * 
 * Clears RAM and prepares memory map. No parameters.
 * 
 * @param none
 * 
 * @returns void 
 */
void mmu_init() {
    memset(&mmu, 0, sizeof(mmu));
    printf("MMU Initialized!.\n");
}


/**
 * @brief mmu_free -
 * frees any dynamically allocated memory by the mmu (the rom for eg) 
 * 
 * @returns void
 */
void mmu_free() {
    if (mmu.rom_data) {
        free(mmu.rom_data;
            mmu.rom_data = NULL;
            printf("ROM Memory freed!.\n");
    }
}



/**
 * @brief Loads a ROM using the rom.c module and integrates it into the MMU.
 */
int mmu_load_rom(const char* filepath) {
    mmu_free(); // Free any previously loaded ROM

    // Delegate the file loading and parsing to the rom.c module
    int result = load_rom(filepath, &mmu.rom_data, &mmu.rom_size, &mmu.mbc_type);

    if (result == 0) { // Check for failure (assuming 0 is failure from your rom.c)
        mmu.rom_data = NULL; // Ensure pointer is null on failure
        return -1;
    }

    printf("ROM loading delegated to rom.c, result integrated into MMU.\n");
    printf("Detected MBC Type: %d\n", mmu.mbc_type);

    // TODO: Initialize the MBC based on the detected type
    // mbc_init(&mmu);
    
    return 0; // Success
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
