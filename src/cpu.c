#include "cpu.h"
#include "mmu.h"
#include "rom.h"
#include "alu.h"

#include <stdbool.h>
#include <stdio.h>

CPU cpu;

// relocated macroes to headerfile

/**
 * @brief cpu_reset - Resets the CPU to its post-BIOS state.
 *
 * Initializes registers and sets PC to 0x0100.
 * 
 * @returns void
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

    cpu.ime = false;
    cpu.ime_enable = false;
    cpu.ime_disable = false;
}

/**
 * @brief cpu_step - Executes a single CPU instruction.
 *
 * @details Fetches, decodes, and executes one instruction at PC.
 * 
 * May modify CPU registers and memory. 
 * 
 * @returns 
 * Returns cycle count
 */
int cpu_step() {
    // Halt if PC goes beyond 64KB or ROM loaded range
    if (cpu.PC == 0xFFFF) { // ((uint32_t)cpu.PC >= 0x10000)
        printf("[HALT] PC out of bounds: 0x%04X\n", cpu.PC);
        cpu.halted = true;
        return 4;
    }

    if (cpu.halted) {
        return 4;
    }

    uint16_t pc = cpu.PC;
    uint8_t opcode = mmu_read(cpu.PC++);
    
    printf("[PC=0x%04X] Opcode 0x%02X | A=0x%02X F=0x%02X B=0x%02X C=0x%02X D=0x%02X E=0x%02X H=0x%02X L=0x%02X SP=0x%04X\n", 
           pc, opcode, cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP
    );

    // CB - Prefixed Bit operations 
    if (opcode == 0xCB) {
        uint8_t cb_opcode = mmu_read(cpu.PC++);

        // special logging for CB_opcodes
        printf("[PC=0x%04X] Opcode 0xCB 0x%02X | A=0x%02X F=0x%02X B=0x%02X C=0x%02X D=0x%02X E=0x%02X H=0x%02X L=0x%02X SP=0x%04X\n", 
           pc, cb_opcode, cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP
        );
        return execute_cb_opcode(cb_opcode);
    }

    // on succesfully executing an instruction, it returns a true value and continues on with the cpu step
    bool success = execute_opcode(opcode); 

    // Apply delayed IME effects AFTER the instruction
    if (cpu.ime_enable) {
        cpu.ime = true;
        cpu.ime_enable = false;
    }

    if (cpu.ime_disable) {
        cpu.ime = false;
        cpu.ime_disable = false;
    }

    return success;
}

// helper functions for PC Incrementing

/**
 * @brief Function to fetch the next 8-bit value
 * 
 * @details Fetches the next 8-bit immediate value and increments the PC
 * 
 * @note the definition scope for this function needs to be static
 * 
 * @param none
 * 
 * @returns the immediate 8-bit value uint8_t 
 */
static uint8_t fetch_d8() {
    return(mmu_read(cpu.PC++));
}


/**
 * @brief Function to fetch the next 16-bit value
 * 
 * @details Fetches the next 16-bit immediate value (little-endian) and advances the PC
 * 
 * @note the definition scope is required to be static
 * 
 * @param none
 * 
 * @returns next 16-bit immediate value 
 */
static uint16_t fetch_d16() {
    uint8_t low = mmu_read(cpu.PC++);
    uint8_t high = mmu_read(cpu.PC++);
    return (high << 8) | low;
}


/**
 * @brief execute_opcode()-
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
        // No operation
        case 0x00: 
            // cpu.PC++;
            break;

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
        // case 0x3E: cpu.A = fetch_d8(); break; // LD A,#
        // case 0x3E implemented further
        case 0x06: cpu.B = fetch_d8(); break; // LD B, n
        case 0x0E: cpu.C = fetch_d8(); break; // LD C, n
        case 0x16: cpu.D = fetch_d8(); break; // LD D, n
        case 0x1E: cpu.E = fetch_d8(); break; // LD E, n
        case 0x26: cpu.H = fetch_d8(); break; // LD H, n
        case 0x2E: cpu.L = fetch_d8(); break; // LD L, n


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
            uint8_t val = fetch_d8();
            // write value at HL register (already defined w macro)
            mmu_write(REG_HL, val);
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

        case 0x0A: cpu.A = mmu_read(REG_BC); break;

        case 0x1A: cpu.A = mmu_read(REG_DE); break;

        // case 0x7E: cpu.A = mmu_read(REG_HL); break;

        // A, (nn)
        case 0xFA:{
            // load from absolute 16-bit address into A
            // PC is at the address of the first operand byte
            uint16_t addr = fetch_d16();
            cpu.A = mmu_read(addr);
            break;
        }

        // A, # case 0x3E
        // Load immediate 8-bit value int A
        case 0x3E:{
            cpu.A = fetch_d8();
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
            uint16_t addr = fetch_d16();
            mmu_write(addr, cpu.A);
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
         * 
         * one byte instruction only, do NOT take an operand
         * from the rom and instead just execute the one byte instruction
         */

        case 0xF2:{
            uint16_t addr = 0xFF00 + cpu.C;
            cpu.A = mmu_read(addr);
            break;
        }

        /**
         * 6. LD (C), A
         * 
         * put value of A into address $FF00 + register C
         * opposite to that of LD A, C
         * 
         * one byte instruction only, do NOT take an operand
         * from the rom and instead just execute the one byte instruction
         *   
         */
        
        case 0xE2:{
            uint16_t addr = 0xFF00 + cpu.C;
            mmu_write(addr, cpu.A);
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

            cpu.H = (hl >> 8) & 0xFF; 
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
            uint16_t offset = fetch_d8(); // taking an offset
            uint16_t addr = 0xFF00 + offset; 

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
            uint8_t offset = fetch_d8();
            cpu.A = mmu_read(0xFF00 + offset);
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
            uint16_t nn = fetch_d16();
            SET_REG_BC(nn);
            break;
        }

        // LD DE, nn
        case 0x11:{
            uint16_t nn = fetch_d16();
            SET_REG_DE(nn);
            break;
        }    
        
        // LD HL, nn
        case 0x21: {    
            uint16_t nn = fetch_d16();
            SET_REG_HL(nn);
            break;
        } 
        
        // LD SP (Stack pointer), nn
        case 0x31:{
            cpu.SP = fetch_d16();
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
            int16_t n = (int8_t)fetch_d8(); // cast to signed int8 first!
            uint16_t sp = cpu.SP;
            uint16_t result = sp + n;

            // set HL to result
            // cpu.H = (result >> 8) & 0xFF;
            // cpu.L = result & 0xFF;
            SET_REG_HL(result);

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
            uint16_t addr = fetch_d16();

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
        case 0xC6: ADD_A(fetch_d8()); break;


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
        case 0xCE: ADC_A(fetch_d8()); break;


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
        case 0xD6: SUB_A(fetch_d8()); break;


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
        case 0xDE: SBC_A(fetch_d8()); break;


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
        case 0xE6: AND_A(fetch_d8()); break;


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
        
        case 0xF6: OR_A(fetch_d8()); break;          //OR A, n



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
        case 0xEE: XOR_A(fetch_d8()); break;


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
        
        case 0xFE: CP_A(fetch_d8()); break;      // CP A, #


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
        case 0x1C: cpu.E = INC(cpu.E); break;   // INC E
        case 0x24: cpu.H = INC(cpu.H); break;   // INC H
        case 0x2C: cpu.L = INC(cpu.L); break;  //INC L

        // INC (HL)
        case 0x34: {
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

        case 0x3D: cpu.A = DEC(cpu.A); break;       // DEC A
        case 0x05: cpu.B = DEC(cpu.B); break;       // DEC B
        case 0x0D: cpu.C = DEC(cpu.C); break;       // DEC C
        case 0x15: cpu.D = DEC(cpu.D); break;       // DEC D
        case 0x1D: cpu.E = DEC(cpu.E); break;       // DEC E
        case 0x25: cpu.H = DEC(cpu.H); break;       // DEC H
        case 0x2D: cpu.L = DEC(cpu.L); break;       // DEC L
        
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


        /* 16-BIT ARITHMETIC OPERATIONS*/

        /**
         * 1. ADD HL, n
         * adds n to HL
         * 
         * use with:
         *  n = BC,DE,HL,SP
         * 
         * flags affected:
            Z - Not affected.
            N - Reset.
            H - Set if carry from bit 11.
            C - Set if carry from bit 15. 
        */

        case 0x09: ADD_HL(REG_BC); break;   // ADD HL, BC
        case 0x19: ADD_HL(REG_DE); break;   // ADD HL, DE
        case 0x29: ADD_HL(REG_HL); break;   // ADD HL, HL
        case 0x39: ADD_HL(cpu.SP); break;   // ADD HL, SP (stakc pointer)
        

        /**
         * 2. ADD SP, n
         * adds n to stack pointer (SP)
         * 
         * use with:
         *  n = one byte signed immediate value (#)
         * 
         * Flags affected:
            Z - Reset.
            N - Reset.
            H - Set or reset according to operation.
            C - Set or reset acc
         */

        case 0xE8: ADD_SP(cpu.SP); break;   //ADD SP, #


        /**
         * 3. INC nn
         * increment register nn 
         * 
         * use with:
         * nn = BC,DE,HL,SP 
         * 
         * Flags affected: none
         */

        // INC BC
        case 0x03: {
            uint16_t bc = REG_BC;
            INC_16(&bc);
            SET_REG_BC(bc); 
            break;
        }

        // INC DE
        case 0x13: {
            uint16_t de = REG_DE;
            INC_16(&de);
            SET_REG_DE(de); 
            break;
        }

        // INC HL
        case 0x23: {
            uint16_t hl = REG_HL;
            INC_16(&hl);
            SET_REG_HL(hl); 
            break;
        }

        case 0x33: INC_16(&cpu.SP); break; // INC SP (stack pointer)
        
        
        /**
         * 4. DEC nn
         * decrement register nn 
         * 
         * use with:
         * nn = BC,DE,HL,SP
         * 
         * Flags affected: none
         */

        // DEC BC
        case 0x0B:{
            uint16_t bc = REG_BC;
            DEC_16(&bc);
            SET_REG_BC(bc);
            break;
        }

        // DEC DE
        case 0x1B:{
            uint16_t de = REG_DE;
            DEC_16(&de);
            SET_REG_DE(de);
            break;
        }

        // DEC HL
        case 0x2B:{
            uint16_t hl = REG_HL;
            DEC_16(&hl);
            SET_REG_HL(hl);
            break;
        }
    
        case 0x3B: DEC_16(&cpu.SP); break;     //DEC SP (stack pointer)


        /* MISCELLANEOUS OPERATIONS */

        /**
         * 1. SWAP - implemented in execute CB opcodes
         * 
         * 
         * 2. DAA decimal adjust register A
         * 
         * this instruction adjusts reg A so that the correct representation of BCD (Binary Coded Decimal) is obtained
         * 
         * Flags affected:
            Z - Set if register A is zero.
            N - Not affected.
            H - Reset.
            C - Set or reset according to operation
         */
        // DAA
        case 0x27: {
            DAA();
            break;
        }

        /**
         * 3. CPL - complement register A (flip all bits)
         * 
         * Flags affected:
            Z - Not affected.
            N - Set.
            H - Set.
            C - Not affected.
         */
        case 0x2F: CPL(); break;


        /**
         * 4. CCF - complement Carry flag
         * if c flag is set, then reset it.
         * If C flag is reset, then set it.
         *
         * Flags affected:
            Z - Not affected.
            N - Reset.
            H - Reset.
            C - Complemented.
         */
        case 0x3F: CCF(); break;


        /** 
         * 5. SCF - set carry flag
         * 
         *  Flags affected:
            Z - Not affected.
            N - Reset.
            H - Reset.
            C - Set.
         */
        case 0x37: SCF(); break;

        /**
         * MISC 9. DI (disable interrupts)
         * this instruction DISABLES interrupts but NOT IMMEDIATELY.
         * Interrupts are disabled after instruction AFTER DI is executed
         * 
         * Flags affected: none
         */
        case 0xF3: cpu.ime_disable = true; break;


        /**
         * MISC 10. EI (enable interrupts)
         * this instruction ENABLES interrupts but not IMMEDIATELY.
         * Interrupts are enabled after instruction AFTER EI is executed
         */
        case 0xFB: cpu.ime_enable = true; break;


        /* 3.3.6 ROTATES AND SHIFTS */

        /**
         * 1. RLC A (what)
         * Rotate A left, Old bit 7 to carry flag 
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 7 data. 
         */

        case 0x07:{
            uint8_t bit7 = (cpu.A >> 7) &0x01;
            cpu.A = (cpu.A << 1) | bit7;

            //flags
            cpu.F = 0; // Z=0, N=0, H=0
            if (bit7) {
                cpu.F |= FLAG_C;
            }
            break;
        }


        /**
         * 2. RLA rotate A left through carry flag
         * 
         * flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 7 data.
         */

        case 0x17: { // RLA
            uint8_t carry = (cpu.F & FLAG_C) ? 1 : 0;
            uint8_t bit7 = (cpu.A >> 7) & 0x01;
            cpu.A = (cpu.A << 1) | carry;

            // Flags
            cpu.F = 0;
            if (bit7) {
                cpu.F |= FLAG_C;
            }

            break;
        }


        /** 
         * 3. RRCA 
         * Rotate A right. old bit 0 to carry flag
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 0 data.
         */ 

        case 0x0F: { // RRCA
            uint8_t bit0 = cpu.A & 0x01;
            cpu.A = (cpu.A >> 1) | (bit0 << 7);

            // Flags
            cpu.F = 0;
            if (bit0) {
                cpu.F |= FLAG_C;
            }

            break;
        }


        /**
         * 4. RRA
         * Rotate A right through carry flag
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 0 data
         */

        case 0x1F: { // RRA
            uint8_t carry = (cpu.F & FLAG_C) ? 1 : 0;
            uint8_t bit0 = cpu.A & 0x01;
            cpu.A = (cpu.A >> 1) | (carry << 7);

            // Flags
            cpu.F = 0;
            if (bit0) {
                cpu.F |= FLAG_C;
            }

            break;
        }



        /* JUMPS (van halen moment)*/
        /**
         * 1. JP, nn
         * jump to address nn 
         * 
         * use with: 
         *  nn = two byte immediate value. (least significant byte first.)
         */
   
        // JP nn
        case 0xC3: {
            cpu.PC = fetch_d16(); // get the address and advance the PC correctly
            break;
        }

        /**
         * 2. JP cc, nn
         * 
         * jump to address n if following conditions are true:
         * cc = NZ, jump if Z flag is reset
         * cc = Z, jump if Z flag is set
         * cc = NC, jump if C flag is reset
         * cc = C, jump if C flag is set
         * 
         * use with:
         * nn = two byte immediate value. (least significant  byte first.)
         * 
         */

        // JP NZ nn
        case 0xC2:{
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_Z) == 0) {
                cpu.PC = addr;
            }

            break;
        }

        // JP Z nn
        case 0xCA:{
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_Z) != 0) {
                cpu.PC = addr;
            } 

            break;
        }

        // JP NC nn
        case 0xD2:{
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_C) == 0) {
                cpu.PC = addr;
            } 

            break;
        }
        
        // JP C nn
        case 0xDA:{
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_C) != 0) {
                cpu.PC = addr;
            }

            break;
        }


        /**
         * 3. JP HL
         * jump to address contained in HL 16-bit register
         * 
         */

        // JP HL 
        case 0xE9: cpu.PC = REG_HL; break;


        /**
         * 4. JR n
         * add n to current address and jump to it
         * 
         * use with:
         * n = one byte signed immediate value 
         */

        // JR n (signed offset)
        case 0x18: {
            int8_t offset = (int8_t)fetch_d8();
            cpu.PC += offset;
            break;
        }

        /**
         * 5. JR cc, n
         * if following condition is true, then add n to current address and jump to it
         * 
         * use with:
         *  n = one byte signed immediate value
            cc = NZ, Jump if Z flag is reset.
            cc = Z, Jump if Z flag is set.
            cc = NC, Jump if C flag is reset.
            cc = C, Jump if C flag is set
         */

        // JR NZ, * opcode 0x20
        case 0x20: {
            int8_t offset = (int8_t)fetch_d8();
            if ((cpu.F & FLAG_Z) == 0) {
                cpu.PC += offset;
            }
            break;
        }

        // JR Z, * opcode 0x28
        case 0x28: {
            int8_t offset = (int8_t)fetch_d8();
            if ((cpu.F & FLAG_Z) != 0) {
                cpu.PC += offset;
            }
            break;
        }

        // JR NC, * opcode 0x30
        case 0x30: {
            int8_t offset = (int8_t)fetch_d8();
            if ((cpu.F & FLAG_C) == 0) {
                cpu.PC += offset;
            }
            break;
        }

        // JR C, * opcode 0x38
        case 0x38: {
            int8_t offset = (int8_t)fetch_d8();
            if ((cpu.F & FLAG_C) != 0) {
                cpu.PC += offset;
            }
            break;
        }


        /* CALLS */

        /**
         * 1. CALL nn 
         * puts address of next instruction onto stack and then jumps to address nn
         * 
         * use with:
         * nn = two byte immediate value (Least Significant byte first)
         */
        // CALL nn
        case 0xCD:{
            uint16_t addr = fetch_d16();
            push16(cpu.PC);
            // replaced the old code for the push16 implementation
            // mmu_write(--cpu.SP, (cpu.PC >> 8));
            // mmu_write(--cpu.SP, (cpu.PC & 0xFF));
            cpu.PC = addr;
            break;
        }

        /**
         * 2. CALL cc, nn 
         * call addr n if following conditions are true:
            cc = NZ, Call if Z flag is reset.
            cc = Z, Call if Z flag is set.
            cc = NC, Call if C flag is reset.
            cc = C, Call if C flag is set.
         * 
         * use with:
         * nn = two byte immediate value (Least significant byte first)
         */

        // CALL NZ, nn
        case 0xC4: {
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_Z) == 0) {
                push16(cpu.PC);
                cpu.PC = addr;
            }
            break;
        }

        // CALL Z, nn
        case 0xCC: {
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_Z) != 0) {
                push16(cpu.PC);
                cpu.PC = addr;
            }
            break;
        }

        // CALL NC, nn
        case 0xD4: {
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_C) == 0) {
                push16(cpu.PC);
                cpu.PC = addr;
            }
            break;
        }

        // CALL C, nn
        case 0xDC: {
            uint16_t addr = fetch_d16();
            if ((cpu.F & FLAG_C) != 0) {
                push16(cpu.PC);
                cpu.PC = addr;
            }
            break;
        }

        /**
         * 3.3.10 RESTARTS
         * 
         * 1. RST n
         * push present address onto stack
         * jump to address $0000 + n
         * 
         * use with:
         *  n = $00,$08,$10,$18,$20,$28,$30,$38
         */
        
        // RST 00H 
        case 0xC7: push16(cpu.PC); cpu.PC = 0x00; break;
        
        // RST 08H 
        case 0xCF: push16(cpu.PC); cpu.PC = 0x08; break;
        
        // RST 10H 
        case 0xD7: push16(cpu.PC); cpu.PC = 0x10; break;
        
        // RST 18H 
        case 0xDF: push16(cpu.PC); cpu.PC = 0x18; break;
        
        // RST 20H 
        case 0xE7: push16(cpu.PC); cpu.PC = 0x20; break;
        
        // RST 28H 
        case 0xEF: push16(cpu.PC); cpu.PC = 0x28; break;
        
        // RST 30H 
        case 0xF7: push16(cpu.PC); cpu.PC = 0x30; break;
        
        // RST 38H 
        case 0xFF: push16(cpu.PC); cpu.PC = 0x38; break;



        /* RETURNS */

        /**
         * 1 RET
         * pop two bytes from stack and jump to that address
         * 
         * opcode: 0xC9
         */
        // RET 
        case 0xC9:{
            uint8_t lo = mmu_read(cpu.SP);
            uint8_t hi = mmu_read(cpu.SP + 1);
            cpu.SP += 2;
            cpu.PC = (hi << 8) | lo;
            break;
        }

        /**
         * 2. RET cc
         * return if the conditions are true:
            cc = NZ, Return if Z flag is reset.
            cc = Z, Return if Z flag is set.
            cc = NC, Return if C flag is reset.
            cc = C, R
         */
        // RET NZ
        case 0xC0: {
            if ((cpu.F & FLAG_Z) == 0) {
                uint8_t low = mmu_read(cpu.SP);
                uint8_t high = mmu_read(cpu.SP + 1);
                cpu.SP += 2;
                cpu.PC = (high << 8) | low;
            }
            break;
        }

        // RET Z
        case 0xC8: {
            if ((cpu.F & FLAG_Z) != 0) {
                uint8_t low = mmu_read(cpu.SP);
                uint8_t high = mmu_read(cpu.SP + 1);
                cpu.SP += 2;
                cpu.PC = (high << 8) | low;
            }
            break;
        }

        // RET NC
        case 0xD0: {           
            if ((cpu.F & FLAG_C) == 0) {
                uint8_t low = mmu_read(cpu.SP);
                uint8_t high = mmu_read(cpu.SP + 1);
                cpu.SP += 2;
                cpu.PC = (high << 8) | low;
            }
            break;
        }

        // RET C
        case 0xD8: {
            if ((cpu.F & FLAG_C) != 0) {
                uint8_t low = mmu_read(cpu.SP);
                uint8_t high = mmu_read(cpu.SP + 1);
                cpu.SP += 2;
                cpu.PC = (high << 8) | low;
            }
            break;
        }


        /**
         * 3. RETI
         * pop two bytes from stack and jump to that address then enable interrupts
         */

         // RETI 
        case 0xD9: {
            uint8_t low = mmu_read(cpu.SP);
            uint8_t high = mmu_read(cpu.SP + 1);
            cpu.SP += 2;
            cpu.PC = (high << 8) | low;
            cpu.ime = true; // enable ime immediately without delay
            break;
        }


        // HALT instruction
        case 0x76: {
            printf("[HALT] HALT instruction encountered at 0x%04X\n", cpu.PC);
            cpu.halted = true;
            return false; // indicate that cpu is halted
        }

        // STOP Instruction
        case 0x10: {
            uint8_t next = mmu_read(cpu.PC++);
            if (next != 0x00) {
                printf("[ERROR] 0x00 instruction expected after STOP");
            }
            cpu.halted = true;
            break;
        }

        default:
            printf("[HALT] Unimplemented opcode: 0x%02X at 0x%04X\n", opcode, cpu.PC);
            cpu.PC--; // Rewind PC for debugging

            cpu.halted = true;
            return false; // Safely halt on unknown opcode   
    }   
    return true;
}

/**
 * @brief Executes CB Opcodes
 * 
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

    
    // array to auto generate the 64 BIT b,r instructions
    // Indexes 0-7 correspond to: B, C, D, E, H, L, (HL), A
    uint8_t* const regs[] = {
        &cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A 
    };

    // switch case for each CB-prefixed opcodes
    switch(opcode) {
        /**
         * 1. SWAP n
         * swap upper and lower nibles of n
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Reset
         */

        case 0x37: cpu.A = SWAP(cpu.A); break;      // SWAP A
        case 0x30: cpu.B = SWAP(cpu.B); break;      // SWAP B
        case 0x31: cpu.C = SWAP(cpu.C); break;      // SWAP C
        case 0x32: cpu.D = SWAP(cpu.D); break;      // SWAP D
        case 0x33: cpu.E = SWAP(cpu.E); break;      // SWAP E
        case 0x34: cpu.H = SWAP(cpu.H); break;      // SWAP H
        case 0x35: cpu.L = SWAP(cpu.L); break;      // SWAP L

        // SWAP (HL)
        case 0x36: {
            uint8_t val = mmu_read(REG_HL);
            uint8_t swap = SWAP(val);

            mmu_write(REG_HL, swap);
            break;
        }

        /**
         * RLC n
         * rotate n left. old bit 7 to carry flag
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 7 data.
         */


        // RLC A 
        case 0x07: { 
            bool carry;
            cpu.A = RLC(cpu.A, &carry);
            cpu.F = 0;

            // if (cpu.A == 0) cpu.F |= FLAG_Z;
            // dont set the Z flag for A case
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC B
        case 0x00: { 
            bool carry;
            cpu.B = RLC(cpu.B, &carry);
            cpu.F = 0;

            if (cpu.B == 0) cpu.F |= FLAG_Z;
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC C
        case 0x01: { 
            bool carry;
            cpu.C = RLC(cpu.C, &carry);
            cpu.F = 0;

            if (cpu.C == 0) cpu.F |= FLAG_Z;
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC D
        case 0x02: { 
            bool carry;
            cpu.D = RLC(cpu.D, &carry);
            cpu.F = 0;

            if (cpu.D == 0) cpu.F |= FLAG_Z;
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC E
        case 0x03: { 
            bool carry;
            cpu.E = RLC(cpu.E, &carry);
            cpu.F = 0;

            if (cpu.E == 0) cpu.F |= FLAG_Z;
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC H
        case 0x04: {
            bool carry;
            cpu.H = RLC(cpu.H, &carry);
            cpu.F = 0;

            if (cpu.H == 0) cpu.F |= FLAG_Z;
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC L
        case 0x05: { 
            bool carry;
            cpu.L = RLC(cpu.L, &carry);
            cpu.F = 0;

            if (cpu.L == 0) cpu.F |= FLAG_Z;
            if (carry)    cpu.F |= FLAG_C;
            break;
        }

        // RLC (HL)
        case 0x06: { 
            bool carry;
            uint8_t val = mmu_read(REG_HL);
            uint8_t result = RLC(val, &carry);
            mmu_write(REG_HL, result);
            cpu.F = 0;

            if (result == 0) cpu.F |= FLAG_Z;
            if (carry)      cpu.F |= FLAG_C;
            break;
        }


        /**
         * 6. RL n 
         * rotate n left through carry flag 
         * 
         * use with:
         *  n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 7 data.
         */

        // RL B
        case 0x10: {
            bool carry_out;
            cpu.B = RL(cpu.B, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            if (cpu.B == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL C
        case 0x11: {
            bool carry_out;
            cpu.C = RL(cpu.C, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            if (cpu.C == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL D
        case 0x12: {
            bool carry_out;
            cpu.D = RL(cpu.D, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            if (cpu.D == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL E
        case 0x13: {
            bool carry_out;
            cpu.E = RL(cpu.E, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            if (cpu.E == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL H
        case 0x14: {
            bool carry_out;
            cpu.H = RL(cpu.H, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            if (cpu.H == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL L
        case 0x15: {
            bool carry_out;
            cpu.L = RL(cpu.L, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            if (cpu.L == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL (HL)
        case 0x16: {
            uint8_t val = mmu_read(REG_HL);
            bool carry_out;
            uint8_t result = RL(val, (cpu.F & FLAG_C), &carry_out);
            mmu_write(REG_HL, result);
            cpu.F = 0;
            if (result == 0) cpu.F |= FLAG_Z;
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        // RL A
        case 0x17: {
            bool carry_out;
            cpu.A = RL(cpu.A, (cpu.F & FLAG_C), &carry_out);
            cpu.F = 0;
            // Z flag is not set for RL A
            if (carry_out)  cpu.F |= FLAG_C;
            break;
        }

        /** 
         * 7. RRC n
         * rotate n right 
         * old bit 0 to carry flag
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 0 data.
         */
        case 0x08: cpu.B = RRC(cpu.B); break;    // RRC B
        case 0x09: cpu.C = RRC(cpu.C); break;    // RRC C 
        case 0x0A: cpu.D = RRC(cpu.D); break;    // RRC D
        case 0x0B: cpu.E = RRC(cpu.E); break;    // RRC E
        case 0x0C: cpu.H = RRC(cpu.H); break;    // RRC H
        case 0x0D: cpu.L = RRC(cpu.L); break;    // RRC L
        
        // RRC (HL)
        case 0x0E: {
            uint16_t addr = REG_HL;
            uint8_t val = mmu_read(addr);
            mmu_write(addr, RRC(val));
            return 16;
        }
        case 0x0F: cpu.A = RRC(cpu.A); break;   // RRC A
        


        /**
         * 8. RR n
         * rotate n right through carry flag
         * 
         * use with:
         *  n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 0 data.
         */
        case 0x18: cpu.B = RR(cpu.B); break;    // RR B
        case 0x19: cpu.C = RR(cpu.C); break;    // RR C
        case 0x1A: cpu.D = RR(cpu.D); break;    // RR D
        case 0x1B: cpu.E = RR(cpu.E); break;    // RR E
        case 0x1C: cpu.H = RR(cpu.H); break;    // RR H
        case 0x1D: cpu.L = RR(cpu.L); break;    // RR L
        
        // RR (HL)
        case 0x1E: {
            uint16_t addr = REG_HL;
            uint8_t val = mmu_read(addr);
            mmu_write(addr, RR(val));
            return 16;
        }
        case 0x1F: cpu.A = RR(cpu.A); break;    // RR A


        /**
         * 9. SLA n
         * shift n left  into carry . Least significant Bit of n set to 0
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 7 data.
         */
        case 0x20: SLA(&cpu.B); break;  // SLA B
        case 0x21: SLA(&cpu.C); break;  // SLA C
        case 0x22: SLA(&cpu.D); break;  // SLA D
        case 0x23: SLA(&cpu.E); break;  // SLA E
        case 0x24: SLA(&cpu.H); break;  // SLA H
        case 0x25: SLA(&cpu.L); break;   // SLA L
        // SLA (HL)
        case 0x26: {
            uint8_t val = mmu_read(REG_HL);
            SLA(&val);
            mmu_write(REG_HL, val);
            break;
        }
        case 0x27: SLA(&cpu.A); break;   // SLA A

        

        /**
         * 10. SRA n
         * shift n right into carry. most significant bit  doesnt change
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 0 data. 
         */
        case 0x28: SRA(&cpu.B); break;  // SRA B
        case 0x29: SRA(&cpu.C); break;  // SRA C
        case 0x2A: SRA(&cpu.D); break;  // SRA D
        case 0x2B: SRA(&cpu.E); break;  // SRA E
        case 0x2C: SRA(&cpu.H); break;  // SRA H
        case 0x2D: SRA(&cpu.L); break;  // SRA L
          // SRA (HL)
        case 0x2E: {
            uint8_t val = mmu_read(REG_HL);
            SRA(&val);
            mmu_write(REG_HL, val);
            break;
        }
        case 0x2F: SRA(&cpu.A); break;  // SRA A


        /**
         * 11. SRL n
         * shift n right into carry. MSB (most significant bit) set to 0
         * 
         * use with:
         * n = A,B,C,D,E,H,L,(HL)
         * 
         * flags affected:
            Z - Set if result is zero.
            N - Reset.
            H - Reset.
            C - Contains old bit 0 data.
         */
        case 0x38: SRL(&cpu.B); break;  // SRL B
        case 0x39: SRL(&cpu.C); break;  // SRL C
        case 0x3A: SRL(&cpu.D); break;  // SRL D
        case 0x3B: SRL(&cpu.E); break;  // SRL E
        case 0x3C: SRL(&cpu.H); break;  // SRL H
        case 0x3D: SRL(&cpu.L); break;  // SRL L
          // SRL (HL)
        case 0x3E: {
            uint8_t val = mmu_read(REG_HL);
            SRL(&val);
            mmu_write(REG_HL, val);
            break;
        }
        case 0x3F: SRL(&cpu.A); break;  // SRL A


        /* 3.3.7 BIT OPCODES */

        /**
         * 1. BIT b, r
         * test bit b in register r
         * 
         * use with:
         *  b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
         * 
         * Flags affected:
            Z - Set if bit b of register r is 0.
            N - Reset.
            H - Set.
            C - Not affected.
         */

        // table for auto-generating all 64opcodes for BIT
        // CB opcodes from 0xC0 to 0x7F
        case 0x40 ... 0x7F: {
            uint8_t bit = (opcode >> 3) & 0x07;     //extracted bit number
            uint8_t index = opcode & 0x07;     // extracted register index

            // register (HL) case
            if (index == 6) {
                uint8_t val = mmu_read(REG_HL);
                BIT(val, bit);
            } else {
                // BIT b, r (registers B, C, D, E, H, L, A)
                BIT(*regs[index], bit);
            }
            break;
        }


        /**
         * 2. SET b, r
         * SETs bit b in register R
         * 
         * use with:
         *  b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
         * 
         * flags affected: none
         */

        // table for auto generating all of the bitcodes for SET
        // CB opcodes 0xC0 to 0xFF
        case 0xC0 ... 0xFF: {
            uint8_t bit = (opcode >> 3) & 0x07; //extract bit index (b)
            uint8_t index = opcode & 0x07; // extract register index (r)

            // register (HL)case
            if (index == 6) {
                uint8_t val = mmu_read(REG_HL);
                SET(&val, bit); // SET bit "bit" in val
                mmu_write(REG_HL, val); // write it back
            } else {
                // register case
                SET((uint8_t*)regs[index], bit); // typecast away const
            }
            break;
        }


        /**
         * 3. RES b, r
         * reset bit b in register r
         * 
         * use with:
         * b = 0 - 7, r = A,B,C,D,E,H,L,(HL)
         * 
         * flags affected: none
         */
        // table to auto generate the CB-prefixed opcodes
        // CB opcodes: 0x80 to 0xBF
        case 0x80 ... 0xBF: {
            uint8_t bit = (opcode >> 3) & 0x07;     //extract bit index (b)
            uint8_t index = opcode & 0x07;          // extract register index (r) 
        
            // register (HL) case
            if (index == 6) {
                uint8_t val = mmu_read(REG_HL);
                RES(&val, bit);     // clear bit 'bit' in val
                mmu_write(REG_HL, val); // write it back
            } else {
                // register case
                RES((uint8_t*)regs[index], bit);    //cast away const
            }
            break;
        }



        default: 
            printf("[CB] Unimplemented opcode: 0x%02X\n", opcode);
            cpu.halted = true;
            return false;
    }
    return true;
}