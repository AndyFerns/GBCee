#ifndef CPU_H
#define CPU_H

#include <stdint.h>

/** 
 * struct CPU:
 * Represents the gameboy CPU registers
 * 
 * Eight 8-bit registers: A, B, C, D, E, F, H, L
 * Two 16-bit registers: SP, PC
 */
typedef struct {
    uint8_t A, F;
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;
    uint16_t PC;
    uint16_t SP;
} CPU;

extern CPU cpu;

/**
 * cpu_reset - Resets the CPU to its post-BIOS state.
 *
 * Initializes registers and sets PC to 0x0100.
 * No parameters, no return value.
 */
void cpu_reset();

/**
 * cpu_step - Executes a single CPU instruction.
 *
 * Fetches, decodes, and executes one instruction at PC.
 * May modify CPU registers and memory.
 * No parameters, no return value.
 */
void cpu_step();

#endif
