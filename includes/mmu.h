// mmu.h  (additions marked //// NEW)
#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h> //// NEW: for size_t

// Maximum rom size: 2MB
#define MAX_ROM_SIZE (2 * 1024 * 1024)

//// NEW: Maximum external RAM size: 32KB (typical upper bound for DMG MBCs)
#define MAX_ERAM_SIZE (32 * 1024)

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

//// NEW: Expose ERAM (so MBC read/write helpers can use it)
extern uint8_t eram[MAX_ERAM_SIZE];

//// NEW: Actual loaded sizes (set by ROM loader), used for bounds checks
extern size_t g_rom_size;
extern size_t g_eram_size;

//// NEW: Setters to be called after ROM header is parsed/ERAM size known
void mmu_set_rom_size(size_t sz);
void mmu_set_eram_size(size_t sz);

#endif
