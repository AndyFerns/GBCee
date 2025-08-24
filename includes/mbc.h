#ifndef MBC_H
#define MBC_H

#include <stdint.h>
#include "mmu.h"

struct mmu_t;

/**
 * @brief initializes the memory bank controller system
 * 
 * @param mmu: a pointer to the main mmu struct
 * 
 * @return void
 */ 
void mbc_init(struct mmu_t* mmu);


/**
 * @brief Handles reads from 0x0000 to 0x7FFF ROM area
 * 
 * @param mmu: pointer to the main memory unit struct
 * @param addr: uint16_t ROM Address
 * 
 * @return Byte read from correctly calculated rom banked area
 */
uint8_t mbc_read_rom(struct mmu_t* mmu, uint16_t addr);


/**
 * @brief Handles write ops from 0x0000 to 0x7FFF ROM area (Bank switching)
 * 
 * @param mmu: pointer to the main mmu struct to modify its sttae
 * @param addr:uint16_t ROM address 
 * @param val:uint8_t value to be written
 * 
 * @return void 
 */
void mbc_write_rom(struct mmu_t mmu, uint16_t addr, uint8_t val);


/**
 * @brief Handles reads  from external ram area (0xA000-0xBFFF)
 * 
 * @param mmu: pointer to the main mmu struct to modify its state
 * @param addr:uint16_t the address to read from
 * 
 * @returns the byte from the correctly calculated ram area
 */
uint8_t mbc_read_ram(struct mmu_t* mmu, uint16_t addr);


/**
 * @brief Handles external RAM writes(0xA000 to 0xBFFF)
 * 
 * @param mmu: pointer to the main mmu struct to modify its sttae
 * @param addr:uint16_t The address to write to
 * @param val:uint8_t the value to be written
 * 
 * @returns void
 */
void mbc_write_ram(struct mmu_t* mmu, uint16_t addr, uint8_t val);

#endif