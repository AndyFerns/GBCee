#ifndef MMU_H
#define MMU_H

#include <stdint.h>

// Maximum rom size: 2MB
#define MAX_ROM_SIZE (2 * 1024 * 1024)

/**
 * init_mmu - 
 * Initializes Main Memory Unit memory regions.
 *
 * Clears RAM and prepares memory map. No parameters.
 */
void init_mmu();

/**
 * mmu_read - 
 * Reads a byte from the specified address.
 * 
 * Parameters:
 * @addr: 16-bit memory address.
 *
 * Return: Value at address.
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

extern uint8_t rom[MAX_ROM_SIZE]; // exposing rom array to main memory

#endif