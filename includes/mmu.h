#ifndef MMU_H
#define MMU_H

#include <stdint.h>


/* MMU Suite */

/**
 * @brief Initializes the MMU and clears all memory regions.
 *
 * This must be called before any other MMU functions are used.
 * 
 * @returns void
 */
void mmu_init();

/**
 * @brief Frees any dynamically allocated memory by the MMU (e.g., the ROM).
 *
 * This should be called when the emulator is shutting down to prevent memory leaks.
 * 
 * @returns void
 */
void mmu_free();


/**
 * @brief mmu_read - 
 * Reads a byte from the specified address.
 * 
 * @param addr 16-bit memory address.
 *
 * @returns Value at address.
 */
uint8_t mmu_read(uint16_t addr);

/**
 * @brief mmu_write - 
 * Writes a byte to the specified address.
 * 
 * @param addr: 16-bit memory address.
 * @param value: Byte to write.
 * 
 * @returns void
 */
void mmu_write(uint16_t addr, uint8_t value);


/**
 * @brief Loads a ROM file from the given path into memory.
 *
 * This function will allocate the necessary memory for the ROM. If a ROM is
 * already loaded, it will be freed first.
 *
 * @param filepath The path to the Game Boy ROM file.
 * @return 0 on success, or -1 on failure (e.g., file not found, memory allocation error).
 */
int mmu_load_rom(const char* filepath);

#endif