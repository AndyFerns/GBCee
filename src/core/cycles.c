#include "cycles.h"

// Base opcode cycles
const uint8_t opcode_cycles[256] = {
    [0x00] = 4,
    [0x01] = 12,
    [0x02] = 8,
    [0x03] = 8,
    [0x04] = 4,
    [0x05] = 4,
    [0x06] = 8,
    [0x07] = 4,
    [0x08] = 20,
    // ... fill rest
};

// CB-prefixed cycles
const uint8_t cb_opcode_cycles[256] = {
    [0x00] = 8,     
    [0x01] = 8,
    [0x02] = 8,
    [0x03] = 8,
    [0x04] = 8,
    [0x05] = 8,
    [0x06] = 16,    // (HL)
    // ...
};