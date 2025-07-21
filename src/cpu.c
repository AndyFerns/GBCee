#include "cpu.h"
#include "mmu.h"
#include <stdio.h>

CPU cpu;

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
    cpu.SP = 0xFFFE; //int 65534
    cpu.PC = 0x0100; // int 256
}

/**
 * cpu_step - See header.
 */
void cpu_step() {
    uint8_t opcode = mmu_read(cpu.PC++);
    printf("[PC=0x%04X] Executing opcode: 0x%02X\n", cpu.PC - 1, opcode);

    switch (opcode) {
        case 0x00: // NOP
            break;

        default:
            printf("Unimplemented opcode: 0x%02X\n", opcode);
            break;
    }
}
