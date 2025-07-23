#include "cpu.h"
#include "mmu.h"
#include "rom.h"

#include <stdbool.h>
#include <stdio.h>

CPU cpu;

/* Helper macros for combined 16-bit registers */
#define REG_BC ((cpu.B << 8) | cpu.C)
#define REG_DE ((cpu.D << 8) | cpu.E)
#define REG_HL ((cpu.H << 8) | cpu.L)

/**
 * cpu_reset - See header.
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
 * cpu_step - See header.
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

    switch (opcode) {
        case 0x00: // No Operation
            // cpu.PC++;
            break;
            
        case 0x3E: // LD A,n
            cpu.A = mmu_read(cpu.PC++);
            break;

        case 0x06: // LD, B, n
            cpu.B = mmu_read(cpu.PC++);
            break;

        case 0x0E: // LD, C, n
            cpu.C = mmu_read(cpu.PC++);
            break;

        case 0x16: // LD D, n
            cpu.D = mmu_read(cpu.PC++);
            break;

        case 0x1E: // LD E, n
            cpu.E = mmu_read(cpu.PC++);
            break;

        case 0x04: // INC B
            cpu.B++;
            // Flags:
            // Z set if result is zero
            // N reset
            // H set if overflow from bit 3
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
            cpu.B--;
            cpu.F |= 0x40; // N

            if (cpu.B == 0) {
                cpu.F |= 0x80;
            }
            break;

        case 0x23: // INC HL
            //right shift H register
            uint16_t hl = (cpu.H << 8) | cpu.L;
            hl++;

            cpu.H = (hl>>8) & 0xFF;
            cpu.L = hl & 0xFF;

            break;

        case 0x76: // HALT instruction
            printf("[HALT] HALT instruction encountered at 0x%04X\n", pc);
            cpu.halted = true;
            return false;

        default:
            printf("[HALT] Unimplemented opcode: 0x%02X at 0x%04X\n", opcode, pc);
            cpu.PC--; // Rewind PC for debugging

            cpu.halted = true;
            return false;   // Safely halt on unknown opcode
    }
    return true;
}
