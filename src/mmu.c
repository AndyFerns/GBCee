#include "mmu.h"
#include <string.h>

// Cartridge ROM region
// DONT KEEP STATIC FOR THE LOVE OF ALL THATS GOOD
uint8_t rom[0x8000]; 
static uint8_t ram[0x10000]; // Full 64KB memory space (simplified)

/**
 * init_mmu - See header.
 */
void init_mmu() {
    memset(ram, 0, sizeof(ram));
}

/**
 * mmu_read - See header.
 */
uint8_t mmu_read(uint16_t addr) {
    if (addr < 0x8000)
        return rom[addr]; // ROM is read-only
    return ram[addr];
}

/**
 * mmu_write - See header.
 */
void mmu_write(uint16_t addr, uint8_t value) {
    if (addr < 0x8000) return; // ROM is read-only
    ram[addr] = value;
}