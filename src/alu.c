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
 *  @brief SWAP swap upper and lower nibbles of 8-bit values
 * 
 * Flags affected:  
 *  Z - Set if result is 0
 *  N - Reset
 *  H - Reset
 *  C - Reset
 * 
 * 
 * 
 */
uint8_t SWAP(uint8_t val) {
    uint8_t result = (val >> 4) | (val << 4);

    cpu.F = 0; // clear all flags

    if (result == 0) {
        cpu.F = FLAG_Z; // set Z flag if result is 0
    }
    return result;
}



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


/**
 * @brief CCF (complement carry flag) 
 * 
 * if C flag is set, then reset it 
 * and if C flag is reset, then set it
 * 
 * Flags affected:
    Z - Not affected.
    N - Reset.
    H - Reset.
    C - Complemented.

 * opcode 0x3F
 *
 * @param none
 * 
 * @returns void
 */
void CCF() {
    cpu.F &= ~(FLAG_N | FLAG_H); // reset N and H flags
    cpu.F ^= FLAG_C; // toggle carry flag (main logic)
}


/**
 * @brief SCF (Set carry flag)
 * 
 * Flags affected:
    Z - Not affected.
    N - Reset.
    H - Reset.
    C - Set.
 *
 * @param none
 * 
 * @returns void
 */
void SCF() {
    cpu.F &= ~(FLAG_N | FLAG_H); // reset N and H flags
    cpu.F |= FLAG_C; // set carry flag
}


/* ROTATES AND SHIFTS */

/**
 * @brief RLC - rotate n left. old bit 7 to carry flag
 * 
 * @param value The byte to rotate
 * @param carry_out Pointer to store the carry flag
 * 
 * @return uint8_t Result of rotation
 */
uint8_t RLC(uint8_t value, bool *carry_out) {
    bool bit7 = (value & 0x80) != 0;                // Check if bit 7 is set
    uint8_t result = (value << 1) | (bit7 ? 1 : 0); // Rotate left, wrap bit7 to bit0

    *carry_out = bit7;
    return result;
}

/**
 * @brief RL - rotate n left through carry
 * 
 * Bit 7 is moved into carry flag.
 * Carry flag is moved into bit 0.
 * 
 * @param value The byte to rotate
 * @param carry_in The current carry flag
 * @param carry_out Pointer to store the new carry flag
 * @return uint8_t The result of rotation
 */
uint8_t RL(uint8_t value, bool carry_in, bool *carry_out) {
    uint8_t bit7 = (value >> 7) & 0x01;
    uint8_t result = (value << 1) | (carry_in ? 1 : 0);
    *carry_out = bit7;
    return result;
}


/**
 * @brief RRC Rotate n right, bit 0 -> carry. Old bit 7 to carry flag
 * 
 * @param value:uint8_t 8 bit register to be rotated
 * 
 * @returns uint8_t The result of rotation
 */
uint8_t RRC(uint8_t value) {
    uint8_t bit0 = value & 0x01;
    uint8_t result = (value >> 1) | (bit0 << 7); // Rotate right

    // Set flags
    cpu.F = 0;
    if (result == 0) cpu.F |= 0x80;  // Z
    if (bit0)        cpu.F |= 0x10;  // C

    return result;
}


/**
 * @brief RR Rotate right through carry
 * 
 * @param value:uint8_t 8 bit register to be rotated
 * 
 * @returns uint8_t The result of rotation
 */
uint8_t RR(uint8_t value) {
    uint8_t carry = (cpu.F & 0x10) ? 1 : 0;   // old carry
    uint8_t bit0 = value & 0x01;
    uint8_t result = (value >> 1) | (carry << 7);

    // Set flags
    cpu.F = 0;
    if (result == 0) cpu.F |= 0x80;  // Z
    if (bit0)        cpu.F |= 0x10;  // C

    return result;
}


/**
 * @brief SLA n - Shift n left by 1. Bit 7 → Carry. Bit 0 = 0.
 * 
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Old bit 7.
 *
 * @param val - Pointer to register or memory to shift
 * 
 * @returns void
 */
void SLA(uint8_t *val) {
    uint8_t old = *val;
    uint8_t result = old << 1;

    // Set flags
    cpu.F = 0;
    if (result == 0) cpu.F |= FLAG_Z;
    if (old & 0x80) cpu.F |= FLAG_C;  // old MSB

    *val = result;
}


/**
 * @brief SRA n - Arithmetic shift right. MSB stays, LSB → Carry.
 * 
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Old bit 0.
 *
 * @param val - Pointer to register or memory to shift
 * 
 * @returns void
 */
void SRA(uint8_t *val) {
    uint8_t old = *val;
    uint8_t msb = old & 0x80;
    uint8_t result = (old >> 1) | msb;

    // Set flags
    cpu.F = 0;
    if (result == 0) cpu.F |= FLAG_Z;
    if (old & 0x01) cpu.F |= FLAG_C;  // old LSB

    *val = result;
}

/**
 * @brief SRL n - Logical shift right. MSB = 0. LSB → Carry.
 * 
 * Flags affected:
 * Z - Set if result is zero.
 * N - Reset.
 * H - Reset.
 * C - Old bit 0.
 *
 * @param val - Pointer to register or memory to shift
 * 
 * @returns void
 */
void SRL(uint8_t *val) {
    uint8_t old = *val;
    uint8_t result = old >> 1;

    // Set flags
    cpu.F = 0;
    if (result == 0) cpu.F |= FLAG_Z;
    if (old & 0x01) cpu.F |= FLAG_C;  // old LSB

    *val = result;
}



/* BIT OPCODES */

/**
 * @brief BIT Checks if a given bit in a register is set or not.
 * 
 * This implements the BIT n, r instruction, which checks if bit `bit` of `value` is 0.
 * 
 * flags affected
 *  Z: Set if bit is 0.
 *  H: Always set.
 *  N: Always reset.
 *  C: Preserved.
 * 
 * @param value The 8-bit register or memory value to test.
 * @param bit   The bit index (0 to 7) to test.
 * 
 * @returns void
 */
void BIT(uint8_t value, uint8_t bit) {
    // Preserve Carry flag, reset N, set H
    cpu.F &= FLAG_C;
    cpu.F |= FLAG_H;
    cpu.F &= ~FLAG_N;

    // Set or reset Z depending on whether bit is 0
    if ((value & (1 << bit)) == 0)
        cpu.F |= FLAG_Z;
    else
        cpu.F &= ~FLAG_Z;
}


/**
 * @brief SET's bit `bit` in `value`.
 * 
 * Flags affected: none
 * 
 * @param value Pointer to register or memory location.
 * @param bit   Bit index to set (0 to 7).
 * 
 * @returns void
 */
void SET(uint8_t* value, uint8_t bit) {
    *value |= (1 << bit);
}


/**
 * @brief Resets (clears) bit `bit` in `value`.
 * 
 * Flags affected: none
 * 
 * @param value Pointer to register or memory location.
 * @param bit   Bit index to reset (0 to 7).
 */
void RES(uint8_t* value, uint8_t bit) {
    *value &= ~(1 << bit);
}