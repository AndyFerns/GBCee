#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h> 


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
