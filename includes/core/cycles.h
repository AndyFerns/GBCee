#ifndef CYCLES_H
#define CYCLES_H

#include <stdint.h>

// Base opcode cycles
extern const uint8_t opcode_cycles[256];

// CB-prefixed opcode cycles
extern const uint8_t cb_opcode_cycles[256];

#endif