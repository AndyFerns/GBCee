#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

/** 
 * struct CPU:
 * Represents the gameboy CPU registers
 * 
 * Eight 8-bit registers: A, B, C, D, E, F, H, L
 * Two 16-bit registers: SP, PC
 */
typedef struct CPU {
    uint8_t A, F; // Accumulator and Flags
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;
    uint16_t PC; // Program Counter
    uint16_t SP; // Stack pointer
    bool halted; // halted flag

    bool ime;           // Master Interrupt enable flag
    bool ime_enable;    // EI (enable interrupts) sets this -> ime becomes true after next instruction 
    bool ime_disable;   // DI (disable interrupts) sets this -> ime becomes false after next instruction
} CPU;

extern CPU cpu;



/* HELPER MACRO DEFINITIONS */

/* Helper macros for combined 16-bit registers */
#define REG_BC ((cpu.B << 8) | cpu.C)
#define REG_DE ((cpu.D << 8) | cpu.E)
#define REG_HL ((cpu.H << 8) | cpu.L)

/* Setter macros for 16-bit operations */ 
#define SET_REG_BC(val) do { cpu.B = ((val) >> 8) & 0xFF; cpu.C = (val) & 0xFF; } while (0)
#define SET_REG_DE(val) do { cpu.D = ((val) >> 8) & 0xFF; cpu.E = (val) & 0xFF; } while (0)
#define SET_REG_HL(val) do { cpu.H = ((val) >> 8) & 0xFF; cpu.L = (val) & 0xFF; } while (0)

/* Flag definitions for Zero(Z), Negative(N), Half-Carry(H), and Carry(C) */
#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10



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
 * No parameters 
 * 
 * Return Value: 
 * Returns cycle count
 */
bool cpu_step();

/**
 * execute_opcode: Execution suite for an opcode
 * 
 * Parameters: 
 * @opcode: 8-bit opcode
 *
 * Return:
 * int
 */
bool execute_opcode(uint8_t opcode);

/**
 * Executes CB Opcodes
 * Separated from execute_opcode for logic
 *
 * @return  bool    
 * returns true if CB opcode is succesfully executed
 * returns false otherwise
 */
bool execute_cb_opcode(uint8_t opcode); 

#endif
