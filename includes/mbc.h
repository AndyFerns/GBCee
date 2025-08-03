#ifndef MBC_H
#define MBC_H

#include <stdint.h>

/**
 * @brief initializes the memory bank controller system
 * 
 * @param type: uint8_t The MBC Type from the ROM header (0x0147)
 * 
 * @return void
 */ 
void mbc_init(uint8_t type);


/**
 * @brief Handles reads from 0x0000 to 0x7FFF ROM area
 * 
 * @param addr: uint16_t ROM Address
 * 
 * @return Byte read from banked ROM
 */
uint8_t mbc_read(uint16_t addr);


/**
 * @brief Handles write ops from 0x0000 to 0x7FFF ROM area (Bank switching)
 * 
 * @param addr:uint16_t ROM address 
 * @param val:uint8_t value to be written
 * 
 * @return void 
 */
void mbc_write(uint16_t addr, uint8_t val);


/**
 * @brief Handles external ram reads
 * 
 * @param addr:uint16_t the address to read from
 * 
 * @returns unsigned integer 8_t
 */
uint8_t mbc_read_ram(uint16_t addr);


/**
 * @brief Handles external RAM writes(0xA000 to 0xBFFF)
 * 
 * @param addr:uint16_t The address to write to
 * @param val:uint8_t the value to be written
 * 
 * @returns void
 */
void mbc_write_ram(uint16_t addr, uint8_t val);

#endif