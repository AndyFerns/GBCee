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
        // case 0x47: cpu.B = cpu.A; break; // LD B, A ---- implemented later in the module
        // case 0x7F is already implemented 

        //for register C
        case 0x48: cpu.C = cpu.B; break; // LD C, B
        case 0x49: break;               // LC C, C (NOP Equivalent)
        case 0x4A: cpu.C = cpu.D; break;// LD C, D
        case 0x4B: cpu.C = cpu.E; break;// LD C, E
        case 0x4C: cpu.C = cpu.H; break;// LD C, H
        case 0x4D: cpu.C = cpu.L; break;// LD C, L
        // case 0x4E: cpu.C = cpu.A; break; // LD C, (HL)
        // case 0x4F: cpu.C = cpu.A; break;

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
        case 0x77: mmu_write(REG_HL, cpu.A); break; // LD (HL), A --- implemented later as well
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
        case 0xFA:{
            // load from absolute 16-bit address into A
            uint8_t low = mmu_read(cpu.PC+ 1);
            uint8_t high = mmu_read(cpu.PC + 2);
            uint16_t addr = ((uint16_t)high << 8) | low;

            cpu.A = mmu_read(addr);
            cpu.PC += 3;
            break;
        }

        // A, # case 0x3E
        // Load immediate 8-bit value int A
        case 0x3E:{
            cpu.A = mmu_read(cpu.PC + 1); // read the immediate value
            cpu.PC += 2; // move pc past opcode and operand

            break;
        }



        /**
         * 4. LD n, A
         * Put value A into n
         * 
         * Use with:
         * 
         * n = A,B,C,D,E,H,L,(BC),(DE),(HL),(nn)
         * nn = two byte immediate value. (LS byte first.)
         */

        
        // case 0x7F: // Already implemented cpu.A = cpu.A   // LD A, A
        case 0x47: cpu.B = cpu.A; break;                     // LD B, A
        case 0x4F: cpu.C = cpu.A; break;                     // LD C, A
        case 0x57: cpu.D = cpu.A; break;                     // LD D, A
        case 0x5F: cpu.E = cpu.A; break;                     // LD E, A
        case 0x67: cpu.H = cpu.A; break;                     // LD H, A
        case 0x6F: cpu.L = cpu.A; break;                     // LD L, A

        case 0x02: mmu_write(REG_BC, cpu.A); break; // LD (BC), A
        case 0x12: mmu_write(REG_DE, cpu.A) ;break; // LD (DE), A
        // case 0x77: ;break; // LD (HL), A -- already implemented as mmu_write(REG_HL, cpu.A)

        // LD (nn = 16 bit immediate address), A 
        case 0xEA: {
            uint8_t low = mmu_read(cpu.PC + 1);
            uint8_t high = mmu_read(cpu.PC  + 2);
            uint16_t addr = ((uint16_t)high << 8) | low;

            mmu_write(addr, cpu.A);
            cpu.PC += 3; //opcode +2 byte address

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

        
        /**
         * 5. LD A, (C)
         * 
         * put value at address $FF00 + register C into A
         * same as LD A, ($FF00 + C) 
         */

        case 0xF2:{
            uint16_t addr = 0xFF00 + cpu.C;
            cpu.A = mmu_read(addr);

            cpu.PC++;
            break;
        }

        /**
         * 6. LD (C), A
         * 
         * put value of A into address $FF00 + register C
         * opposite to that of LD A, C
         *   
         */
        
        case 0xE2:{
            uint16_t addr = 0xFF00 + cpu.C;
            mmu_write(addr, cpu.A);

            cpu.PC++;
            break;
        }

        /**
         * 7. LD A, (HLD)
         * 
         * same as LDD A, (HL)
         * 
         * 
         * 8. LD A, (HL-)
         * 
         * same as LDD A, (HL)
         * 
         * 
         * 9. LDD A, (HL)
         * 
         * --- all 3 aliases: 
         * put value of HL into A
         * Decrement HL
         * 
         * same as: LD A, (HL)  -  DEC HL
         */

        case 0x3A: {
            //LD A, (HL-)
            //LDD A, (HL)
            uint16_t hl = REG_HL;
            cpu.A = mmu_read(hl);
            hl--; //decrement

            cpu.H = (hl >> 8) &0xFF;
            cpu.L = hl &0xFF;
            break;
        }


        /**
         * 10. LD (HLD), A
         * same as LDD (HL), A
         * 
         * 11. LD (HL-), A
         * same as LDD (HL), A
         * 
         * 12. LDD (HL), A
         * puts A into memory address (HL)
         * reverse of 
         * decrement HL
         * 
         * same as LD (HL), A  -  DEC HL 
         */
         
        case 0x32:{
            uint16_t hl = REG_HL;
            mmu_write(hl, cpu.A);
            hl--; //decrement step

            cpu.H = (hl << 8) & 0xFF; 
            cpu.L = hl & 0xFF;
            break;
        }

        /**
         * 13. LD A, (HLI) --- HL increment
         * same as LDI A, (HL)
         * 
         * 14. LD A, (HL+)
         * same as LDI A, (HL)
         * 
         * 15. LDI A, (HL)
         * put value at address HL into A
         * increment HL
         * 
         * same as LD A, (HL)  -  INC HL
         * 
         */

        case 0x2A:{
            uint16_t hl = REG_HL;
            cpu.A = mmu_read(hl);

            hl++; //increment step

            cpu.H = (hl >> 8) & 0xFF;
            cpu.L = hl & 0xFF;

            break;
        }

        /**
         * 16. LD (HLI), A
         * same as LDI (HL), A
         * 
         * 17. LD (HL+), A
         * same as LDI (HL), A 
         * 
         * 18. LDI (HL), A
         * puts A into memory address HL
         * increment HL
         * 
         * same as LD (HL), A --- INC HL
         * 
         */

        case 0x22:{
            uint16_t hl = REG_HL;
            mmu_write(hl, cpu.A);

            hl++; //increment step

            cpu.H = (hl >> 8) & 0xFF;
            cpu.L = hl & 0xFF;

            break;
        }


        /**
         * 19. LDH (n), A
         * puts A into memory address $FF00 + n
         * 
         * use with: 
         * n = one byte immediate
         */

        case 0xE0:{
            uint16_t n = mmu_read(cpu.PC++); // taking n as offset
            uint16_t addr = 0xFF00 + n; 

            mmu_write(addr, cpu.A);
            break;
        }

        /**
         * 20. LDH A, (n)
         * put memory address $FF00 + n into A
         * reverse of instruction 19
         * 
         * use with:
         * n = one byte immediate value
         */

        case 0xF0:{
            uint16_t n = mmu_read(cpu.PC++);
            uint16_t addr = 0xFF00 + n;

            cpu.A = mmu_read(addr);
            break;
        }


        
        /* 16-bit load ooperations */
        
        /**
         * 1. LD n, nn
         * put value nn into n
         * 
         * use with:
         * n = BC,DE,HL,SP
         * nn = 16 bit immediate value
         * 
         */

        // LD BC, nn
        case 0x01:{ 
            uint16_t nn = mmu_read(cpu.PC++);
            nn |= mmu_read(cpu.PC++) << 8;
            cpu.B = (nn >> 8) & 0xFF;
            cpu.C = nn & 0xFF;
            break;
        }

        // LD DE, nn
        case 0x11:{
            uint16_t nn = mmu_read(cpu.PC++);
            nn |= mmu_read(cpu.PC++) << 8;
            cpu.D = (nn >> 8) & 0xFF;
            cpu.E = nn & 0xFF;
            break;
        }    
        
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


        /**
         * 2. LD SP, HL
         * puts value at HL into SP (stack Pointer)
         * 
         * 8 cycles
         */
        
        // LD SP, HL 
        case 0xF9:{
            // cpu.SP = (cpu.H << 8) | cpu.L; 
            cpu.SP = REG_HL; // macro predefined for consistency
            break;
        }


        /**
         * 3. LD HL, SP + n
         * same as LDHL SP, n
         * 
         * 4. LDHL SP, n
         * Puts SP + n effective address into HL
         *
         * use with:
         * n = one byte signed immediate value
         * 
         * flags affected:
         * Z = Reset
         * N = Reset
         * H = Set or reset according to operation
         * C = Set or reset acc to op
         */

        // signed offset arithmetic !!!
        case 0xF8: {
            int16_t n = (int8_t)mmu_read(cpu.PC++); // cast to signed int8 first!
            uint16_t sp = cpu.SP;
            uint16_t result = sp + n;

            // set HL to result
            cpu.H = (result >> 8) & 0xFF;
            cpu.L = result & 0xFF;

            //clear Z and N flags
            cpu.F &= ~(FLAG_Z | FLAG_N);

            //set Half carry (H) and carry (c) flags based on lower byte addition
            // use same logic to add signed int to unsigned int
            uint16_t temp = (sp ^ n ^ result) & 0xFFFF;

            if ((temp & 0x10) != 0) {
                cpu.F |= FLAG_H;
            } else {
                cpu.F &= ~FLAG_H;
            }
            if ((temp & 0x100) != 0) {
                cpu.F |= FLAG_C;
            } else {
                cpu.F &= ~FLAG_C;
            }

            break;
        }

        /**
         * 5. LD (nn), SP
         * put stack pointer (SP) at addr n
         * 
         * use with:
         * nn = two byte immediate address
         * 
         * 20 cycles
         */


        case 0x08: {
            // read immediate 16-bit address nn
            // it is a littl;e endian
            uint16_t addr = mmu_read(cpu.PC) | (mmu_read(cpu.PC + 1) << 8);
            cpu.PC += 2;

            // store stack pointer val at addr nn
            // little endian -- low byte first
            mmu_write(addr, cpu.SP & 0xFF);     // SP Low byte
            mmu_write(addr + 1, (cpu.SP >> 8) & 0xFF); // SP high byte

            break;
        }

        // STACK OPERATIONS PUSH AND POP

        /**
         * 6. PUSH nn
         * 
         *  push register pair nn onto stack
         * decrement SP (Stack Pointer) twice
         * 
         * use with:
         * nn = AF, BC, DE, HL
         * 
         * HIGH BYTE FIRST
         * LOW BYTE NEXT
         */

        // PUSH AF
        case 0xF5: {
            mmu_write(--cpu.SP, cpu.A);
            mmu_write(--cpu.SP, cpu.F & 0xF0);  // mask off lower 4 bits (always 0 on hardware)
            break;
        }

        // PUSH BC
        case 0xC5:{
            mmu_write(--cpu.SP, cpu.B);
            mmu_write(--cpu.SP, cpu.C);
            break;
        }

        // PUSH DE
        case 0xD5:{
            mmu_write(--cpu.SP, cpu.D);
            mmu_write(--cpu.SP, cpu.E);
            break;
        }

        // PUSH HL
        case 0xE5:{
            mmu_write(--cpu.SP, cpu.H);
            mmu_write(--cpu.SP, cpu.L);
            break;
        }


        /**
         * 7. POP nn
         * 
         * pop off two bytes off stack into register pair nn
         * increment stack pointer SP twice every iteration
         * 
         * use with:
         * nn = AF, BC, DE, HL
         * 
         *  LOW BYTE FIRST
         *  HIGH BYTE NEXT
         * 
         */

        // POP AF
        case 0xF1: {
            cpu.F = mmu_read(cpu.SP++) & 0xF0;
            cpu.A = mmu_read(cpu.SP++);
            break;
        }


        // POP BC
        case 0xC1:{
            cpu.C = mmu_read(cpu.SP++);
            cpu.B = mmu_read(cpu.SP++);
            break;
        }

        // POP DE
        case 0xD1:{
            cpu.E = mmu_read(cpu.SP++);
            cpu.D = mmu_read(cpu.SP++);
            break;
        }

        // POP HL   
        case 0xE1:{
            cpu.L = mmu_read(cpu.SP++);
            cpu.H = mmu_read(cpu.SP++);
            break;
        }

        // -------------------------------- //
        /* ARITHMETIC LOGIC UNIT OPERATIONS */
        // -------------------------------- //

        /* 8-BIT ARITHMETIC LOGIC OPERATIONS*/
        /**
         * 1. ADD A, n
         * add n to A
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Set if carry from bit 3.
            C - Set if car
         */

        case 0x87: ADD_A(cpu.A); break;     // ADD A, A
        case 0x80: ADD_A(cpu.B); break;     // ADD A, B
        case 0x81: ADD_A(cpu.C); break;     // ADD A, C
        case 0x82: ADD_A(cpu.D); break;     // ADD A, D
        case 0x83: ADD_A(cpu.E); break;     // ADD A, E
        case 0x84: ADD_A(cpu.H); break;     // ADD A, H
        case 0x85: ADD_A(cpu.L); break;     // ADD A, L

        case 0x86: ADD_A(mmu_read(REG_HL)); break;     // ADD A, (HL)
        
        // ADD A, #
        case 0xC6:{ 
            uint8_t val = mmu_read(cpu.PC++);
            ADD_A(val);
            break; 
        }


        /**
         * 2. ADC A, n
         * add n + carry flag to A
         * 
         * use with:
         * n = A,B, C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Set if carry from bit 3.
            C - Set if carry from bit 7.
         */

        case 0x8F:ADC_A(cpu.A); break;      // ADC A, A
        case 0x88:ADC_A(cpu.B); break;      // ADC A, B
        case 0x89:ADC_A(cpu.C); break;      // ADC A, C
        case 0x8A:ADC_A(cpu.D); break;      // ADC A, D
        case 0x8B:ADC_A(cpu.E); break;      // ADC A, E
        case 0x8C:ADC_A(cpu.H); break;      // ADC A, H
        case 0x8D:ADC_A(cpu.L); break;      // ADC A, L

        case 0x8E:ADC_A(mmu_read(REG_HL)); break;      // ADC A, (HL)
        
        // ADC A, #
        case 0xCE: {
            uint8_t val = mmu_read(cpu.PC++);
            ADC_A(val);
            break;
        }    



        /**
         * 3. SUB n
         * subtract n from A
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Set.
            H - Set if no borrow from bit 4.
            C - Set if no borrow

         * SUB B, A  ==  Subtract B from A
         */
        
        case 0x97: SUB_A(cpu.A); break;     // SUB A, A
        case 0x90: SUB_A(cpu.B); break;     // SUB B, A
        case 0x91: SUB_A(cpu.C); break;     // SUB C, A
        case 0x92: SUB_A(cpu.D); break;     // SUB D, A
        case 0x93: SUB_A(cpu.E); break;     // SUB E, A
        case 0x94: SUB_A(cpu.H); break;     // SUB H, A
        case 0x95: SUB_A(cpu.L); break;     // SUB L, A

        case 0x96: SUB_A(mmu_read(REG_HL)); break;     // SUB (HL), A

        // SUB #, A
        case 0xD6: {
            uint8_t val = mmu_read(cpu.PC++);
            SUB_A(val);
            break;
        }


        /**
         * 4. SBC_A
         * subtract n + carry flag from A
         * 
         * use with: 
         * n = A,B,C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Set.
            H - Set if no borrow from bit 4.
            C - Set if no borro
         */

        case 0x9F: SBC_A(cpu.A); break;     //SBC A, A
        case 0x98: SBC_A(cpu.B); break;     //SBC A, B
        case 0x99: SBC_A(cpu.C); break;     //SBC A, C
        case 0x9A: SBC_A(cpu.D); break;     //SBC A, D
        case 0x9B: SBC_A(cpu.E); break;     //SBC A, E
        case 0x9C: SBC_A(cpu.H); break;     //SBC A, H
        case 0x9D: SBC_A(cpu.L); break;     //SBC A, L

        case 0x9E: SBC_A(mmu_read(REG_HL)); break;     //SBC A, (HL)

        // SBC A, #
        // not in the gameboy manual ?!?!?!
        case 0xDE:{
            uint8_t val = mmu_read(cpu.PC++);
            SBC_A(val);
            break;
        }


        /**
         * 5. AND n
         * logically AND n with A 
         * 
         * result in A register
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL),# 
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Set.
            C - Reset.
         * 
         */

        case 0xA7: AND_A(cpu.A); break;    // AND A 
        case 0xA0: AND_A(cpu.B); break;    // AND B 
        case 0xA1: AND_A(cpu.C); break;    // AND C 
        case 0xA2: AND_A(cpu.D); break;    // AND D 
        case 0xA3: AND_A(cpu.E); break;    // AND E 
        case 0xA4: AND_A(cpu.H); break;    // AND H 
        case 0xA5: AND_A(cpu.L); break;    // AND L 

        case 0xA6: AND_A(mmu_read(REG_HL)); break;    // AND (HL) 

        // AND A, #
        case 0xE6: {
            uint8_t val = mmu_read(cpu.PC++);
            AND_A(val);
            break;
        }


        /**
         * 6. OR n
         * logical OR with register A
         * 
         * result is stored in A
         * 
         * use with:
         * n =  A,B,C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Rese
         */
        
        case 0xB7: OR_A(cpu.A); break;          //OR A, A
        case 0xB0: OR_A(cpu.B); break;          //OR A, B
        case 0xB1: OR_A(cpu.C); break;          //OR A, C
        case 0xB2: OR_A(cpu.D); break;          //OR A, D
        case 0xB3: OR_A(cpu.E); break;          //OR A, E
        case 0xB4: OR_A(cpu.H); break;          //OR A, H
        case 0xB5: OR_A(cpu.L); break;          //OR A, L
        
        case 0xB6: OR_A(mmu_read(REG_HL)); break;          //OR A, (HL)
        
        case 0xF6: OR_A(mmu_read(cpu.PC++)); break;          //OR A, (HL)



        /**
         * 7. XOR n
         * logical exclusive OR n with register A
         * 
         * store result in A
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Reset.
         */

        case 0xAF: XOR_A(cpu.A); break;     // XOR A, A
        case 0xA8: XOR_A(cpu.B); break;     // XOR A, B
        case 0xA9: XOR_A(cpu.C); break;     // XOR A, C
        case 0xAA: XOR_A(cpu.D); break;     // XOR A, D
        case 0xAB: XOR_A(cpu.E); break;     // XOR A, E
        case 0xAC: XOR_A(cpu.H); break;     // XOR A, H
        case 0xAD: XOR_A(cpu.L); break;     // XOR A, L
        
        case 0xAE: XOR_A(mmu_read(REG_HL)); break;     // XOR A, (HL)

        //XOR A, *
        case 0xEE: XOR_A(mmu_read(cpu.PC++)); break;


        /**
         * 8. CP n
         *  compare A with n. 
         * 
         * this is basically an A - n subtraction instruction but the results
         * are not stored and discarded instead
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL),#
         * 
         * Flags affected:
            Z - Set if result is zero. (Set if A = n.)
            N - Set.
            H - Set if no borrow from bit 4.
            C - Set for no borrow. (Set if A < n.)
         */
        case 0xBF: CP_A(cpu.A); break;      // CP A, A
        case 0xB8: CP_A(cpu.B); break;      // CP A, B
        case 0xB9: CP_A(cpu.C); break;      // CP A, C
        case 0xBA: CP_A(cpu.D); break;      // CP A, D
        case 0xBB: CP_A(cpu.E); break;      // CP A, E
        case 0xBC: CP_A(cpu.H); break;      // CP A, H
        case 0xBD: CP_A(cpu.L); break;      // CP A, L
        
        case 0xBE: CP_A(mmu_read(REG_HL)); break;      // CP A, (HL)
        
        case 0xFE: CP_A(mmu_read(cpu.PC++)); break;      // CP A, #


        /* INCREMENT AND DECREMENT OPERATORS*/
        
        /* 8-BIT INCREMENT OPERATIONS*/

        /**
         * 9. INC n
         * increment register n
         * 
         * use with:
         *  n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Set if carry from bit 3.
            C - Not affected
         */

        case 0x3C: cpu.A = INC(cpu.A); break;  // INC A
        case 0x04: cpu.B = INC(cpu.B); break;   // INC B
        case 0x0C: cpu.C = INC(cpu.C); break;   // INC C
        case 0x14: cpu.D = INC(cpu.D); break;   // INC D
        case 0x1C: cpu.E = INC(cpu.E); break;   // INC e
        case 0x24: cpu.H = INC(cpu.H); break;   // INC F
        case 0x2C: cpu.L = INC(cpu.L); break;  //INC L

        // INC (HL)
        case 0x23: {
            uint8_t val = mmu_read(REG_HL);
            val = INC(val);
            mmu_write(REG_HL, val);
            break;
        }

        
        /* DECREMENT OPERATIONS */
        /**
         * 10. DEC n
         * decrement register n
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if reselt is zero.
            N - Set.
            H - Set if no borrow from bit 4.
            C - Not affect
         */

        case 0x3D: cpu.A = DEC(cpu.A); break;       // DEC A, A
        case 0x05: cpu.B = DEC(cpu.B); break;       // DEC A, B
        case 0x0D: cpu.C = DEC(cpu.C); break;       // DEC A, C
        case 0x15: cpu.D = DEC(cpu.D); break;       // DEC A, D
        case 0x1D: cpu.E = DEC(cpu.E); break;       // DEC A, E
        case 0x25: cpu.H = DEC(cpu.H); break;       // DEC A, H
        case 0x2D: cpu.L = DEC(cpu.L); break;       // DEC A, L
        
        // DEC A, (HL)
        case 0x35: {
            uint8_t val = mmu_read(REG_HL);
            val = DEC(val);
            mmu_write(REG_HL, val);
            break;
        }

        // older logic: 
        // case 0x05: {
        //     uint8_t prev = cpu.B;
        //     cpu.B--;

        //     cpu.F &= FLAG_C;        //preserve C (carry) always, clear other flags
        //     cpu.F |= FLAG_N;
            
        //     if (cpu.B == 0) {
        //         cpu.F |= FLAG_Z;
        //     }
            
        //     if ((prev & 0x0F) == 0x00) {
        //         cpu.F |= FLAG_H;
        //     }
        //     break;
        // }
        
        case 0x76: // HALT instruction
            printf("[HALT] HALT instruction encountered at 0x%04X\n", cpu.PC);
            cpu.halted = true;
            return false; // indicate that cpu is halted


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