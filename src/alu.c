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

// SUB r
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

// AND r
/**
 * @brief AND_R- applying logical AND between A and val
 * 
 * @param val:uint8_t 8-bit value to AND A with
 * 
 * @return void
 */
void AND_A(uint8_t val) {
    cpu.A &= val;
    cpu.F = FLAG_H; // half-carry is ALWAYS set
    if (cpu.A == 0) {
        cpu.F |= FLAG_Z;
    }
}

// OR r, XOR r, CP r yet to be implemented