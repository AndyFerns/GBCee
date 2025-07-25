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
 * @returns 
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
 * @param opcode 8-bit operation code to be executed
 *
 * @return  returns true on success and false on halt/unknown instruction
 */
bool execute_opcode(uint8_t opcode) {
    /*
        Flags:
        Z set if result is zero
        N reset
        H set if overflow from bit 3
    */
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
        case 0x18: {
            int8_t offset = (int8_t)mmu_read(cpu.PC++);
            cpu.PC += offset;
            break;
        }

        // CALL nn
        case 0xCD:{
            uint16_t addr = mmu_read(cpu.PC) | (mmu_read(cpu.PC + 1) << 8);
            cpu.PC += 2;
            mmu_write(--cpu.SP, (cpu.PC >> 8));
            mmu_write(--cpu.SP, (cpu.PC & 0xFF));
            cpu.PC = addr;
            break;
        }

        // RET 
        case 0xC9:{
            uint8_t lo = mmu_read(cpu.SP);
            uint8_t hi = mmu_read(cpu.SP + 1);
            cpu.SP += 2;
            cpu.PC = (hi << 8) | lo;
            break;
        }

        /* 8-bit Load operations */

        /** 
         * 1. LD nn, n
         * 
         * Description: Puts value nn into n
         * 
         * Use With: B,C,D,E,H,L,BC,DE,HL,SP
         *
         * LD B,n  06  
         * LD C,n  0E  
         * LD D,n  16  
         * LD E,n  1E  
         * LD H,n  26   
         * LD L,n  2E 
         */
        // case 0x3E: cpu.A = mmu_read(cpu.PC++); break; // LD A,#
        // case 0x3E implemented further
        case 0x06: cpu.B = mmu_read(cpu.PC++); break; // LD B, n
        case 0x0E: cpu.C = mmu_read(cpu.PC++); break; // LD C, n
        case 0x16: cpu.D = mmu_read(cpu.PC++); break; // LD D, n
        case 0x1E: cpu.E = mmu_read(cpu.PC++); break; // LD E,n
        case 0x26: cpu.H = mmu_read(cpu.PC++); break; // LD H, n
        case 0x2E: cpu.L = mmu_read(cpu.PC++); break; // LD L, n

        /** 
         * 2. LD r1, r2
         * 
         * Description: Puts value r2 into r1
         * 
         * Use With: 
         * r1,r2 = A,B,C,D,E,H,L,(HL)
         */
        
        // LD r1, r2 : Copy between two 8-bit resistors
        // for register A
        case 0x7F: break;               //(NOP Equivalent) LD A, A
        case 0x78: cpu.A = cpu.B; break; // LD A, B
        case 0x79: cpu.A = cpu.C; break; // LD A, C
        case 0x7A: cpu.A = cpu.D; break; // LD A, D
        case 0x7B: cpu.A = cpu.E; break; // LD A, E
        case 0x7C: cpu.A = cpu.H; break; // LD A, H
        case 0x7D: cpu.A = cpu.L; break; // LD A, L
        
        // for register B
        case 0x40: break;               // LC B, B(NOP Equivalent)
        case 0x41: cpu.B = cpu.C; break; // LD B, C
        case 0x42: cpu.B = cpu.D; break;// LD B, D
        case 0x43: cpu.B = cpu.E; break;// LD B, E
        case 0x44: cpu.B = cpu.H; break;// LD B, H
        case 0x45: cpu.B = cpu.L; break;// LD B, L
        case 0x47: cpu.B = cpu.A; break; // LD B, A
        // case 0x7F is already implemented 

        //for register C
        case 0x48: cpu.C = cpu.B; break; // LD C, B
        case 0x49: break;               // LC C, C (NOP Equivalent)
        case 0x4A: cpu.C = cpu.D; break;// LD C, D
        case 0x4B: cpu.C = cpu.E; break;// LD C, E
        case 0x4C: cpu.C = cpu.H; break;// LD C, H
        case 0x4D: cpu.C = cpu.L; break;// LD C, L
        // case 0x4E: cpu.C = cpu.A; break; // LD C, (HL)
        //case 0x4F already implemented

        //for register D
        case 0x50: cpu.D = cpu.B; break; // LD D, B
        case 0x51: cpu.D = cpu.C; break;// LD D, C
        case 0x52: break;               // LC D, D (NOP Equivalent)
        case 0x53: cpu.D = cpu.E; break;// LD D, E
        case 0x54: cpu.D = cpu.H; break;// LD D, H
        case 0x55: cpu.D = cpu.L; break;// LD D, L
        // case 0x56: cpu.D = cpu.A; break; // LD D, (HL)

        //for register E
        case 0x58: cpu.E = cpu.B; break; // LC E, B
        case 0x59: cpu.E = cpu.C; break; // LD E, C
        case 0x5A: cpu.E = cpu.D; break;// LD E, D
        case 0x5B: break;               // LD E, E (NOP Equivalent)
        case 0x5C: cpu.E = cpu.H; break;// LD E, H
        case 0x5D: cpu.E = cpu.L; break;// LD E, L
        // case 0x5E: LD E, (HL)

        //for register H
        case 0x60: cpu.H = cpu.B; break; // LC H, B
        case 0x61: cpu.H = cpu.C; break; // LD H, C
        case 0x62: cpu.H = cpu.D; break;// LD H, D
        case 0x63: cpu.H = cpu.E; break;// LD H, E
        case 0x64: break;               // LD H, H (NOP Equivalent)
        case 0x65: cpu.H = cpu.L; break;// LD H, L
        // case 0x66: LD H, (HL)

        //for register L
        case 0x68: cpu.L = cpu.B; break;// LC L, B
        case 0x69: cpu.L = cpu.C; break; // LD L, C
        case 0x6A: cpu.L = cpu.D; break;// LD L, D
        case 0x6B: cpu.L = cpu.E; break;// LD L, E
        case 0x6C: cpu.L = cpu.H; break;// LD L, H
        case 0x6D: break;               // LD L, L (NOP Equivalent)
        // case 0x6E:  LD L, (HL)
        
        // LD (HL), n
        case 0x77: mmu_write(REG_HL, cpu.A); break; // LD (HL), A
        case 0x70: mmu_write(REG_HL, cpu.B); break; // LD (HL), B
        case 0x71: mmu_write(REG_HL, cpu.C); break; // LD (HL), C
        case 0x72: mmu_write(REG_HL, cpu.D); break; // LD (HL), D
        case 0x73: mmu_write(REG_HL, cpu.E); break; // LD (HL), E
        case 0x74: mmu_write(REG_HL, cpu.H); break; // LD (HL), H
        case 0x75: mmu_write(REG_HL, cpu.L); break; // LD (HL), L

        // LD (HL), n-- 12 cycle count
        case 0x36: {
            uint8_t val = mmu_read(cpu.PC++);
            uint16_t addr = (cpu.H << 8) | cpu.L;

            mmu_write(addr, val);
            cpu.PC += 2;
            break;
        }

        /**
         * 3. LD A, n
         * 
         * Put value of n into A
         * 
         * use with: 
         * n = A,B,C,D,E,H,L,(BC),(DE),(HL),(nn),#
         * nn = two byte immediate value. (LS byte first.)
         * 
         */

        // 0x7F - 0x7D already implemented

        // A, (BC)
        // case 0x0A:
        //     // load from memory address pointed by BC into A
        //     uint16_t addr = ((uint8_t)cpu.B << 8) | cpu.C;
        //     cpu.A = mmu_read(addr);

        //     cpu.PC++;
        //     break;

        case 0x0A: cpu.A = mmu_read(REG_BC); break;

        // A, (DE)
        // case 0x1A:
        //     // load from memory address pointed by DE into A
        //     uint16_t addr = ((uint8_t)cpu.D << 8) | cpu.E;
        //     cpu.A = mmu_read(addr);

        //     cpu.PC++;
        //     break;

        case 0x1A: cpu.A = mmu_read(REG_DE); break;


        // A, (HL)
        // case 0x7E:
        //     // load from memory address pointed by HL into A
        //     uint16_t addr = ((uint8_t)cpu.H << 8) | cpu.L;
        //     cpu.A = mmu_read(addr);

        //     cpu.PC++;
        //     break;

        // case 0x7E: cpu.A = mmu_read(REG_HL); break;

        // A, (nn)
        case 0xFA:
            // load from absolute 16-bit address into A
            uint8_t low = mmu_read(cpu.PC+ 1);
            uint8_t high = mmu_read(cpu.PC + 2);
            uint16_t addr = ((uint16_t)high << 8) | low;

            cpu.A = mmu_read(addr);
            cpu.PC += 3;
            break;

        // A, # case 0x3E
        // Load immediate 8-bit value int A
        case 0x3E:
            cpu.A = mmu_read(cpu.PC + 1); // read the immediate value
            cpu.PC += 2; // move pc past opcode and operand

            break;
        
        // 16-bit load ooperations
        // LD HL, nn
        case 0x21: {    
            uint16_t nn = mmu_read(cpu.PC++);
            nn |= mmu_read(cpu.PC++) << 8; // logical OR + right shift by 8 bits
            cpu.H = (nn >> 8) & 0xFF;
            cpu.L = nn & 0xFF;
            break;
        } 
        
        // LD SP (Stack pointer), nn
        case 0x31:{
            uint16_t nn = mmu_read(cpu.PC++);
            nn |= mmu_read(cpu.PC++) << 8;
            cpu.SP = nn; 
            break;
        }
        
        // LD SP, HL 
        case 0xF9:{
            // cpu.SP = (cpu.H << 8) | cpu.L; 
            cpu.SP = REG_HL; // macro predefined for consistency
            break;
        }
        
        /* Load - Store Instructions */
        // LD n, (HL)
        case 0x7E: cpu.A = mmu_read(REG_HL); break; // LD A (HL)    
        case 0x46: cpu.B = mmu_read(REG_HL); break; // LD B (HL)
        case 0x4E: cpu.C = mmu_read(REG_HL); break; // LD C (HL)
        case 0x56: cpu.D = mmu_read(REG_HL); break; // LD D (HL)
        case 0x5E: cpu.E = mmu_read(REG_HL); break; // LD E (HL)
        case 0x66: cpu.H = mmu_read(REG_HL); break; // LD H (HL)
        case 0x6E: cpu.L = mmu_read(REG_HL); break; // LD L (HL)
        
        // Increment and Decrement operators;
        
        /* INCREMENT OPERATIONS*/
        // INCB
        case 0x04:{
            uint8_t prev = cpu.B;
            cpu.B++; // increment step

            cpu.F &= FLAG_C;    // preserver C (carry), clear thhe other flags
            if (cpu.B == 0) {
                cpu.F |= FLAG_Z;
            } 
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        // INC C
        case 0x0C: {
            uint8_t prev = cpu.C;
            cpu.C++;

            cpu.F &= FLAG_C;
            if (cpu.C == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break; 
        }

        // INC D
        case 0x14: {
            uint8_t prev = cpu.D;
            cpu.D++;

            cpu.F &= FLAG_C;
            if (cpu.D == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break; 
        }

        // INC E
        case 0x1C: {
            uint8_t prev = cpu.E;
            cpu.E++;

            cpu.F &= FLAG_C;
            if (cpu.E == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break; 
        }
        
        // INC H
        case 0x24:{
            uint8_t prev = cpu.H;
            cpu.H++;

            cpu.F &= FLAG_C;
            if (cpu.H == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break;
        }
        
        // INC L
        case 0x2C:{
            uint8_t prev = cpu.L;
            cpu.L++;
            
            cpu.F &= FLAG_C;
            if (cpu.L == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        // INC A
        case 0x3C:{    
            uint8_t prev = cpu.A;
            cpu.A++;

            cpu.F &= FLAG_C;
            if (cpu.A == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x0F) {
                cpu.F |= FLAG_H;
            }
            break; 
        }

        // INC HL
        case 0x23: {
            //right shift H register
            uint16_t hl = (cpu.H << 8) | cpu.L;
            hl++;
            
            cpu.H = (hl>>8) & 0xFF;
            cpu.L = hl & 0xFF;

            break;
        }
        
        /* DECREMENT OPERATIONS */
        //DEC B
        case 0x05: {
            uint8_t prev = cpu.B;
            cpu.B--;

            cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
            cpu.F |= FLAG_N;
            
            if (cpu.B == 0) {
                cpu.F |= FLAG_Z;
            }
            
            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        // DEC C
        case 0x0D: {
            uint8_t prev = cpu.C;
            cpu.C--;
            
            cpu.F &= FLAG_C;        // preserve C, clear others
            cpu.F |= FLAG_N;

            if (cpu.C == 0) {
                cpu.F |= FLAG_Z;
            }
            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        //DEC D
        case 0x15: {
            uint8_t prev = cpu.D;
            cpu.D--;

            cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
            cpu.F |= FLAG_N;

            if (cpu.D == 0) {
                cpu.F |= FLAG_Z;
            }

            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }
        
        //DEC E
        case 0x1D: {
            uint8_t prev = cpu.E;
            cpu.E--;
            
            cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
            cpu.F |= FLAG_N;

            if (cpu.E == 0) {
                cpu.F |= FLAG_Z;
            }

            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        //DEC H
        case 0x25: {
            uint8_t prev = cpu.H;
            cpu.H--;
            
            cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
            cpu.F |= FLAG_N;

            if (cpu.H == 0) {
                cpu.F |= FLAG_Z;
            }

            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        //DEC L
        case 0x2D:{
            uint8_t prev = cpu.L;
            cpu.L--;
            
            cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
            cpu.F |= FLAG_N;

            if (cpu.L == 0) {
                cpu.F |= FLAG_Z;
            }

            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        //DEC A
        case 0x3D: {
            uint8_t prev = cpu.A;
            cpu.A--;

            cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
            cpu.F |= FLAG_N;

            if (cpu.A == 0) {
                cpu.F |= FLAG_Z;
            }

            if ((prev & 0x0F) == 0x00) {
                cpu.F |= FLAG_H;
            }
            break;
        }

        // DEC HL
        case 0x35: {
            //right shift H register
            uint16_t hl = (cpu.H << 8) | cpu.L;
            hl--;
            
            cpu.H = (hl>>8) & 0xFF;
            cpu.L = hl & 0xFF;

            break;
        }

        /* Stack Operations*/
        // PUSH B, C
        case 0xC5:
            mmu_write(--cpu.SP, cpu.C);
            mmu_write(--cpu.SP, cpu.B);
            break;

        // POP B, C
        case 0xC1:
            cpu.C = mmu_read(cpu.SP++);
            cpu.B = mmu_read(cpu.SP++);
            break;
        
        case 0x76: // HALT instruction
            printf("[HALT] HALT instruction encountered at 0x%04X\n", cpu.PC);
            cpu.halted = true;
            return false; // indicate that cpu is halted

        // Arithmetic Logic Unit Operations (ALU Ops)
        case 0x80: ADD_A(cpu.B); break;     // ADD A, B
        case 0x81: ADD_A(cpu.C); break;     // ADD A, C

        case 0x90: SUB_A(cpu.B); break;     // SUB B from A

        case 0xA0: AND_A(cpu.B); break;     // AND A, B
        case 0xB0: OR_A(cpu.B); break;     // OR A, B
        case 0xB8: XOR_A(cpu.B); break;     // XOR A, B

        case 0xA8: CP_A(cpu.B); break;     // Compare CP A, B

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
    /*  Set Z if bit is clear
        Clear N
        Set H
        Preserve C
        According to gameboy spec BIT instructions
    */
    switch(opcode) {
        // BIT 0, B
        case 0x40: 
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.B & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;

        // BIT 0, C
        case 0x41:
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.C & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;

        // BIT 0, D
        case 0x42:
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.D & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;
        
        // BIT 0, E
        case 0x43:
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.E & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;

        // BIT 0, H
        case 0x44:
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.H & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;

        // BIT 0, L
        case 0x45:
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.L & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;

        // // BIT 0, (HL)
        // case 0x46:
        //     printf("Not implemented");
        //     return false;
        //     break;

        // BIT 0, A
        case 0x47:
            cpu.F &= FLAG_C; // preserve C
            cpu.F |= FLAG_H; // always set H
            cpu.F &= ~FLAG_N; // clear N

            if ((cpu.A & (1 << 0)) == 0)
                cpu.F |= FLAG_Z;
            else
                cpu.F &= ~FLAG_Z;
            break;

        // SET
        // yet to be implemented

        // RES 
        // yet to be implemented

        default: 
            printf("[CB] Unimplemented opcode: 0x%02X\n", opcode);
            cpu.halted = true;
            return false;
    }
    return true;
}