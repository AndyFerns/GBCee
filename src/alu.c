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
    if ((cpu.A & 0x0F) + (val & 0x0F) + carry > 0x0F) {
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
    if ((cpu.A & 0xFF) < (val & 0x0F) + carry) {
        cpu.F |= FLAG_H;
    }
    if (cpu.A < (val + carry)) {
        cpu.F |= FLAG_C;
    }
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



/* LOGICAL OPERATIONS */
/**
 * AND r
 * OR r
 * XOR r
 */


// AND r
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
 * DAA()
 * CPL()
 */

/**
 *  1. SWAP n -TBD
 */

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