#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h>

// Maximum rom size: 2MB
#define MAX_ROM_SIZE (2 * 1024 * 1024)  // 2MB

// maximum external RAM (eram) size: 32KB (upper bound for DMG MBCs)
#define MAX_ERAM_SIZE (32 * 1024)   // 32 KB 

/* Memory exposition suite */

// exposing rom array to main memory
extern uint8_t rom[MAX_ROM_SIZE]; 

// exposing eram to main memory (For MBC read/write helper ops)
extern uint8_t eram[MAX_ERAM_SIZE];

// actual loaded sizes (these are set by the rom loader)
// used for bound checks
extern size_t g_rom_size;
extern size_t g_ram_size;


/**
 * @brief init_mmu - 
 * Initializes Main Memory Unit memory regions.
 *
 * Clears RAM and prepares memory map. 
 * 
 * @param none
 * 
 * @returns void
 */
void init_mmu();

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

// TODO: calling and implementing setters
// setters to be called after ROM header is parsed and ERAM size is known
void mmu_set_rom_size(size_t sz);
void mmu_set_eram_size(size_t sz);

#endif