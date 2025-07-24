#include "cpu.h"
#include "mmu.h"
#include "rom.h"
#include "alu.h"

#include <stdbool.h>
#include <stdio.h>

CPU cpu;

/* Helper macros for combined 16-bit registers */
#define REG_BC ((cpu.B << 8) | cpu.C)
#define REG_DE ((cpu.D << 8) | cpu.E)
#define REG_HL ((cpu.H << 8) | cpu.L)

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
void cpu_reset() {
    cpu.A = 0x01; // int 1
    cpu.F = 0xB0; // int 176
    cpu.B = 0x00; // int 0
    cpu.C = 0x13; // int 19
    cpu.D = 0x00; // int 0
    cpu.E = 0xD8; // int 216
    cpu.H = 0x01; // int 1
    cpu.L = 0x4D; // int 77
    // Stack Pointer
    cpu.SP = 0xFFFE; //int 65534
    // Program counter
    cpu.PC = 0x0100; // int 256
    cpu.halted = false;
}

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
bool cpu_step() {
    // Halt if PC goes beyond 64KB or ROM loaded range
    if (cpu.PC == 0xFFFF) { // ((uint32_t)cpu.PC >= 0x10000)
        printf("[HALT] PC out of bounds: 0x%04X\n", cpu.PC);
        return false;
    }

    if (cpu.halted) {
        return false;
    }

    uint16_t pc = cpu.PC;
    uint8_t opcode = mmu_read(cpu.PC++);
    
    printf("[PC=0x%04X] Opcode 0x%02X | A=0x%02X F=0x%02X B=0x%02X C=0x%02X D=0x%02X E=0x%02X H=0x%02X L=0x%02X SP=0x%04X\n", 
           pc, opcode, cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP
    );

    // CB - Prefixed Bit operations 
    if (opcode == 0xCB) {
        uint8_t cb_opcode = mmu_read(cpu.PC++);
        return execute_cb_opcode(cb_opcode);
    }

    return execute_opcode(opcode);
}

/**
 * execute_opcode()-
 * Decodes and executes given 8-bit opcode 
 *
 * Return value: int  
 * returns true on success
 * false on halt/unknown instruction
 */
bool execute_opcode(uint8_t opcode) {
    switch (opcode) {
        case 0x00: // No Operation
            // cpu.PC++;
            break;

        // program execution flow transferred to new memory address "n n"       
        // JP nn
        case 0xC3: {
            uint16_t addr = mmu_read(cpu.PC) | (mmu_read(cpu.PC + 1) << 8);
            cpu.PC = addr;
            break;
        }

        // Branching Instructions
        // JR n (signed offset)
        case 0x18: 
            int8_t offset = (int8_t)mmu_read(cpu.PC++);
            cpu.PC += offset;
            break;

        // CALL nn
        case 0xCD:
            uint16_t addr = mmu_read(cpu.PC) | (mmu_read(cpu.PC + 1) << 8);
            cpu.PC += 2;
            cpu.SP -= 2;
            mmu_write(cpu.SP, cpu.PC & 0xFF);
            mmu_write(cpu.SP + 1, cpu.PC >> 8); 
            cpu.PC = addr;
            break;

        // RET 
        case 0xC9:
            uint8_t lo = mmu_read(cpu.SP);
            uint8_t hi = mmu_read(cpu.SP + 1);
            cpu.SP += 2;
            cpu.PC = (hi << 8) | lo;
            break;
        

        // Resistor Load operations
        case 0x3E: cpu.A = mmu_read(cpu.PC++); break; // LD A,n
        case 0x06: cpu.B = mmu_read(cpu.PC++); break; // LD B, n
        case 0x0E: cpu.C = mmu_read(cpu.PC++); break; // LD C, n
        case 0x16: cpu.D = mmu_read(cpu.PC++); break; // LD D, n
        case 0x1E: cpu.E = mmu_read(cpu.PC++); break; // LD E,n
        case 0x26: cpu.H = mmu_read(cpu.PC++); break; // LD H, n
        case 0x2E: cpu.L = mmu_read(cpu.PC++); break; // LD L, n


        // LD r1, r2 : Copy between two 8-bit resistors
        case 0x78: cpu.A = cpu.B; break; // LD A, B
        case 0x79: cpu.A = cpu.C; break; // LD A, C
        case 0x7A: cpu.A = cpu.D; break; // LD A, D
        case 0x7B: cpu.A = cpu.E; break; // LD A, E
        case 0x7C: cpu.A = cpu.F; break; // LD A, H
        case 0x7D: cpu.A = cpu.L; break; // LD A, L
        // LD A, A 
        case 0x7F: break; //(NOP Equivalent) 


        case 0x47: cpu.B = cpu.A; break; // LD B, A
        case 0x41: cpu.B = cpu.C; break; // LD B, C
        case 0x42: cpu.B = cpu.D; break;// LD B, D
        case 0x43: cpu.B = cpu.E; break;// LD B, E
        case 0x44: cpu.B = cpu.H; break;// LD B, H
        case 0x45: cpu.B = cpu.L; break;// LD B, L
        // LD B, B
        case 0x40: break; // (NOP Equivalent)


        // Flags:
        // Z set if result is zero
        // N reset
        // H set if overflow from bit 3


        // Increment and Decrement operators;
        // INC/DEC B
        case 0x04: // INC B
            cpu.B++;
            cpu.F &= 0x10; //preserve carry
            if (cpu.B == 0) {
                cpu.F |= 0x80; //Z
            }
            if ((cpu.B & 0x0F) == 0x00) {
                cpu.F |= 0x20; //H
            }
            break;

        case 0x05: //DEC B
            if ((cpu.B & 0x0F) == 0) {
                cpu.F |= 0x20; //H
            }
            cpu.B--; // decrement step
            cpu.F |= 0x40; // N

            if (cpu.B == 0) {
                cpu.F |= 0x80; // Z
            }
            break;

        // INC / DEC C
        case 0x0C: // INC C
            cpu.C++;
            cpu.F &= 0x10;
            if (cpu.C == 0) {
                cpu.F |= 0x80; // Z
            }
            if ((cpu.C & 0x0F) == 0x00) {
                cpu.F |= 0x20; // H
            }
            break; 
        
        case 0x0D: // DEC C
            cpu.F &= 0x10; // Preserve Carry
            if ((cpu.C & 0x0F) == 0) {
                cpu.F |= 0x20; // H
            }
            cpu.C--; // Decrement step
            cpu.F |= 0x40; // N
            if (cpu.C == 0x00) {
                cpu.F |= 0x80;
            } // Z
            break;

        case 0x23: // INC HL
            //right shift H register
            uint16_t hl = (cpu.H << 8) | cpu.L;
            hl++;

            cpu.H = (hl>>8) & 0xFF;
            cpu.L = hl & 0xFF;

            break;

        /* Load - Store Instructions */
        // LD n, (HL)
        case 0x7E: cpu.A = mmu_read(REG_HL); break;
        case 0x46: cpu.B = mmu_read(REG_HL); break;
        case 0x4E: cpu.C = mmu_read(REG_HL); break;
        case 0x56: cpu.D = mmu_read(REG_HL); break;
        case 0x5E: cpu.E = mmu_read(REG_HL); break;
        case 0x66: cpu.H = mmu_read(REG_HL); break;
        case 0x6E: cpu.L = mmu_read(REG_HL); break;

        // LD (HL), n
        case 0x77: mmu_write(REG_HL, cpu.A); break;
        case 0x70: mmu_write(REG_HL, cpu.B); break;
        case 0x71: mmu_write(REG_HL, cpu.C); break;
        case 0x72: mmu_write(REG_HL, cpu.D); break;
        case 0x73: mmu_write(REG_HL, cpu.E); break;
        case 0x74: mmu_write(REG_HL, cpu.H); break;
        case 0x75: mmu_write(REG_HL, cpu.L); break;
        
        case 0x76: // HALT instruction
            printf("[HALT] HALT instruction encountered at 0x%04X\n", cpu.PC);
            cpu.halted = true;
            return 0;

        // Arithmetic Logic Unit Operations (ALU Ops)
        case 0x80: ADD_A(cpu.B); break;     // ADD A, B
        case 0x81: ADD_A(cpu.C); break;     // ADD A, C

        case 0x90: SUB_A(cpu.B); break;     // SUB B from A

        case 0xA0: AND_A(cpu.B); break;     // ADD B and A
        case 0xB0: OR_A(cpu.B); break;     // OR B and A
        case 0xB8: XOR_A(cpu.B); break;     // XOR B, A
        case 0xA8: CP_A(cpu.B); break;     // Compare CP B, A

        default:
            printf("[HALT] Unimplemented opcode: 0x%02X at 0x%04X\n", opcode, cpu.PC);
            cpu.PC--; // Rewind PC for debugging

            cpu.halted = true;
            return false; // Safely halt on unknown opcode   
    }   
    return true;
}

/**
 * Executes CB Opcodes
 * Separated from execute_opcode for logic
 *
 * @return  bool    
 * returns true if CB opcode is succesfully executed
 * returns false otherwise
 */
bool execute_cb_opcode(uint8_t opcode) {
    switch(opcode) {
        // BIT 0, B
        case 0x40: 
            cpu.F = (cpu.B & (1 << 0)) ? (cpu.F & FLAG_C) : (FLAG_H | FLAG_Z);
            break;

        // BIT 0, C
        case 0x41:
            cpu.F = (cpu.C & (1 << 0)) ? (cpu.F & FLAG_C) : (FLAG_H | FLAG_Z);
            break;

        // SET
        default: 
            printf("[CB] Unimplemented opcode: 0x%02X\n", opcode);
            cpu.halted = true;
            return false;
    }
    return true;
}