#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h> 
#include <stdbool.h>

#include "rom.h"

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
    bool ram_enabled;
    int current_rom_bank;
    int current_ram_bank;
    int mbc1_mode;

    // Timer registers and internal state
    uint16_t internal_timer;    // 16-bit counter for DIV
    uint8_t tima;               // 0xFF05 - TIMA register counter
    uint8_t tma;                // 0xFF06 - Timer modulo
    uint8_t tac;                // 0xFF07 - Timer control
} mmu_t;

/**
 * @brief mmu_init - 
 * Initializes Main Memory Unit memory regions.
 *
 * Clears RAM and prepares memory map. No parameters.
 * 
 * @returns void
 */
void mmu_init();

/**
 * @brief mmu_free -
 * frees any dynamically allocated memory by the mmu (the rom for eg) 
 * 
 * @returns void
 */
void mmu_free();

/**
 * @brief mmu_read - Reads a byte from the specified address.
 * 
 * @param addr 16-bit memory address.
 *
 * @returns uint8_t Value at address.
 */
uint8_t mmu_read(uint16_t addr);

/**
 * mmu_write - 
 * Writes a byte to the specified address.
 * 
 * Parameters: -
 * @addr: 16-bit memory address.
 * @value: Byte to write.
 * 
 * Return: void
 */
void mmu_write(uint16_t addr, uint8_t value);


/**
 * @brief Loads a ROM file from the given path into memory.
 *
 * @param filepath The path to the Game Boy ROM file.
 * @return 0 on success, or -1 on failure (e.g., file not found).
 */
int mmu_load_rom(const char* filepath);

// removed external loading for bound checks

#endif
