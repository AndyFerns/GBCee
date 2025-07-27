#include <stdio.h>
#include <stdint.h>

#include "cpu.h"

/* Flag bit Masks 
 * GameBoy CPU:
 * 
 * Zero (Z)
 * Subtract (N)
 * Half-Carry (H)
 * Carry (C)
 */
#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

/* Helper macros for combined 16-bit registers */
#define REG_BC ((cpu.B << 8) | cpu.C)
#define REG_DE ((cpu.D << 8) | cpu.E)
#define REG_HL ((cpu.H << 8) | cpu.L)


/* ARITHMETIC OPERATIONS */
/** 
 * ADD_A
 * ADC_A
 * SUB_A
 * SBC_A
 * CP_A
 */

// Addition operator ADD
/**
 * ADD_A - Adds val to register A.
 * 
 * @param  val:uint8_t 8-bit value to add to A
 * 
 * @return  void
 */
void ADD_A(uint8_t val) {
    uint16_t result = cpu.A + val;
    cpu.F = 0;
    if ((result & 0xFF) == 0) {
        cpu.F |= FLAG_Z;
    }
    if ((cpu.A & 0x0F) + (val & 0x0F) > 0x0F) {
        cpu.F |= FLAG_H;
    }
    if (result > 0xFF) {
        cpu.F |= FLAG_C;
    }
    cpu.A = result & 0xFF;
}


/**
 * @brief Add val + carry flag to A
 * 
 * @param val:uint8_t 8-bit value to add to A
 * 
 * @return void
 */
void ADC_A(uint8_t val) {
    uint8_t carry = (cpu.F & FLAG_C) ? 1 : 0;
    uint16_t result = cpu.A + val + carry;

    cpu.F = 0; // reset flag

    if ((result & 0xFF) == 0) {
        cpu.F |= FLAG_Z;
    }
    if (((cpu.A & 0x0F) + (val & 0x0F) + carry) & 0x10) {
        cpu.F |= FLAG_H;
    }
    if (result > 0xFF) {
        cpu.F |= FLAG_C;
    }

    cpu.A = result & 0xFF;
}


// SUB n
/**
 * @brief SUB_A - Subtracts value from register A
 *
 * @param @val: 8-bit value to sub from A
 * 
 * @return  void    
 */
void SUB_A(uint8_t val) { 
    cpu.F = FLAG_N;
    if ((cpu.A & 0x0F) < (val & 0x0F)) {
        cpu.F |= FLAG_H;
    }
    if (cpu.A < val) {
        cpu.F |= FLAG_C; 
    }
    cpu.A -= val;

    if (cpu.A == 0) {
        cpu.F |= FLAG_Z;
    }
}


/**
 * @brief Sub val + carry flag to A
 * 
 * @param val:uint8_t 8-bit value to add to A
 * 
 * @return void
 */
void SBC_A(uint8_t val) {
    uint8_t carry = (cpu.F & FLAG_C) ? 1 : 0;
    uint16_t result = cpu.A - val - carry;

    cpu.F = FLAG_N;

    if ((result & 0xFF) == 0) {
        cpu.F |= FLAG_Z;
    }
    if ((cpu.A & 0x0F) < ((val & 0x0F) + carry)) {
        cpu.F |= FLAG_H;
    }
    if (cpu.A < (val + carry)) {
        cpu.F |= FLAG_C;
    }
    cpu.A = result & 0xFF;
}

/**
 * simple comparison between A and val.
 * 
 * result is not stored.
 * 
 * @param val 8-bit integer to compare A with
 * 
 * @return void
 */
void CP_A(uint8_t val) {
    cpu.F = FLAG_N;
    if ((cpu.A & 0x0F) < (val & 0x0F)) {
        cpu.F |= FLAG_H;
    }
    if (cpu.A < val) {
        cpu.F |= FLAG_C;
    }
    uint8_t result = cpu.A - val;
    if (result == 0) {
        cpu.F |= FLAG_Z;
    }
}


// 16- BIT ARITHMETIC OPERATIONS

/**
 * @brief ADD_HL adds 16-bit value to HL resistor
 * 
 *  * Flags affected:
 *   N - Reset
 *   H - Set if carry from bit 11
 *   C - Set if carry from bit 15
 *   Z - Not affected
 * 
 * @param val 16-bit register value (BC, DE, HL, SP)
 * 
 * @returns void
 */
void ADD_HL(uint16_t val) {
    uint32_t result = REG_HL + val;

    //reset N
    cpu.F &= ~FLAG_H;

    // Check half carry from bit 11
    if (((REG_HL & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF)
        cpu.F |= FLAG_H;
    else
        cpu.F &= ~FLAG_H;

    // Check full carry from bit 15
    if (result > 0xFFFF)
        cpu.F |= FLAG_C;
    else
        cpu.F &= ~FLAG_C;

    // set H and L values as expected
    cpu.H = (result >> 8) & 0xFF;
    cpu.L = result & 0xFF;
}

/**
 * @brief ADD_SP - Adds signed 8-bit immediate value to SP
 * 
 * Flags affected:
 *   Z - Reset
 *   N - Reset
 *   H - Set if carry from bit 3
 *   C - Set if carry from bit 7
 * 
 * @param val signed 8-bit value to add to SP
 * 
 * @returns void
 */
void ADD_SP(int8_t val) {
    uint16_t sp = cpu.SP;
    uint16_t result = sp + val;

    cpu.F = 0; // Reset Z and N

    // Half-carry check (bit 3)
    if (((sp & 0xF) + (val & 0xF)) > 0xF)
        cpu.F |= FLAG_H;

    // Full-carry check (bit 7)
    if (((sp & 0xFF) + (val & 0xFF)) > 0xFF)
        cpu.F |= FLAG_C;

    cpu.SP = result;
}


/**
 * @brief INC_16 Increment 16-bit register nn
 * 
 * nn = BC,DE,HL,SP 
 * 
 * Flags affected: none
 * 
 * @param 
 * 
 * @return 
 */
void INC_16(uint16_t *regist) {
    *regist += 1;
}


/**
 * @brief DEC_16 decrement 16-bit register nn
 * 
 * nn = BC,DE,HL,SP 
 * 
 * Flags affected: none
 * 
 * @param 
 * 
 * @return 
 */
void DEC_16(uint16_t *regist) {
    *regist -= 1;
}



/* LOGICAL OPERATIONS */
/**
 * AND r
 * OR r
 * XOR r
 */

/**
 * @brief AND_R- applying logical AND between A and val
 * 
 * @param val:uint8_t 8-bit value to AND A with
 * 
 * @return void
 */
void AND_A(uint8_t val) {
    cpu.A &= val; //apply logical AND
    cpu.F = FLAG_H; // half-carry is ALWAYS set
    if (cpu.A == 0) {
        cpu.F |= FLAG_Z;
    }
    // N and C are implicitly reset by setting F = FLAG_H | FLAG_Z
}

/** 
 * @brief Applying logical OR between A and val
 *
 * @param val:uint8_t 8-bit value to OR A with
 * 
 * @return void
 */
void OR_A(uint8_t val) {
    cpu.A |= val; //apply logical OR
    cpu.F = 0; // set half carry to 0
    if (cpu.A == 0) {
        cpu.F |= FLAG_Z;
    }
}


/**
 * @brief XOR_A Logical XOR betn A and val
 * 
 * @param val:uint8_t 8-bit unsigned integer value to XOR A with
 *
 * @return  void
 */
void XOR_A(uint8_t val) {
    cpu.A ^= val; // XOR setting step
    cpu.F = 0; //set half carry to 0
    if (cpu.A == 0) {
        cpu.F |= FLAG_Z;
    }
}


/* MISCELLANEOUS OPERATORS */

/**
 * SWAP n
 * INC()
 * DEC()
 * DAA()
 * CPL()
 */

/**
 *  1. SWAP n -TBD
 */


/**
 * @brief INC - Increments 8-bit register
 * 
 * increments the given value and updates Z,H,N flags. Preserves the C flag
 * 
 * @param val: 8-bit register value to increment
 * 
 * @return uint8_t: incremented value
 */
uint8_t INC(uint8_t val) {
    uint8_t result = val + 1;

    // clear Z, H, N flags
    // preserve carry flag
    cpu.F &= FLAG_C;

    if (result == 0) {
        cpu.F |= FLAG_Z;
    }
    // half carry if lower nible overflows
    if ((val & 0x0F) == 0x0F) {
        cpu.F |= FLAG_H;
    }
    // N cleared already by &= FLAG_C
    return result;
}



/**
 * @brief DEC- Decrement an 8-bit resistor
 * 
 * Decrements the given value and updates Z, H, N flags. Preserves C Flag
 * 
 * @param val 8-bit resistor value to decrement
 * 
 * @return uint8_t decremented value
 */
uint8_t DEC(uint8_t val) {
    uint8_t result = val - 1;

    // clear z, h
    // set n
    // preserve C
    cpu.F &= FLAG_C;
    cpu.F |= FLAG_N;

    if (result == 0) {
        cpu.F |= FLAG_Z;
    }
    // half borrow:
    // if lower nibble borrows (0x10 -> 0x0F)
    if ((val & 0x0F) == 0x00) {
        cpu.F |= FLAG_H;
    }
    return result;
}


/**
 * @brief DAA
 * decimal adjust register A
 * this instr adjusts reg A so that the correct representation of 
 * Binary Coded Decimal (BCD) is obtained
 * 
 * uses opcode 0x27
 * 
 * @param none
 * 
 * @return none
 */
void DAA() {
    uint8_t a = cpu.A;
    uint8_t adjust = 0;
    uint8_t carry = 0;

    // after ADD
    if (!(cpu.F & FLAG_N)) {
        if ((cpu.F & FLAG_H) || (a & 0xFF) > 9) {
            adjust |= 0x06;
        }

        if ((cpu.F & FLAG_C) || a > 0x99) {
            adjust |= 0x60;
            carry = 1;
        }
        a += adjust;
    } else {
        // after SUB
        if (cpu.F & FLAG_H) {
            adjust |= 0x06;
        }
        if (cpu.F & FLAG_C) {
            adjust |= 0x60;
        }
        a -= adjust;
    }
    //  FLAGS UPDATE (Z, C), and update A
    cpu.F &= ~(FLAG_Z | FLAG_H); // clear Z and H
    if (a == 0) cpu.F |= FLAG_Z;
    if (carry || (cpu.F & FLAG_C)) cpu.F |= FLAG_C;

    cpu.A = a;
}


/**
 * @brief complement A register (flips all bits)
 * 
 * Flags affected:
 * Z - Not affected.
 * N - Set.
 * H - Set.
 * C - Not affected.
 * 
 * uses opcode 0x2F
 * 
 * @param none
 * 
 * @return void
 * 
 */
void CPL() {
    cpu.A = ~cpu.A;
    cpu.F |= FLAG_N | FLAG_H;
}