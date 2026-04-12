#pragma once

#include <stdio.h>

/*
========================================
        DEBUG CONFIGURATION
========================================
Flip these to 1 or 0 as needed
*/

#ifndef DEBUG_MASTER
#define DEBUG_MASTER 1   // master switch (0 = kill ALL logs)
#endif

#ifndef DEBUG_CPU
#define DEBUG_CPU 1
#endif

#ifndef DEBUG_CB
#define DEBUG_CB 1
#endif

#ifndef DEBUG_MMU
#define DEBUG_MMU 1
#endif

#ifndef DEBUG_PPU
#define DEBUG_PPU 0
#endif

#ifndef DEBUG_TIMER
#define DEBUG_TIMER 0
#endif

/*
========================================
        INTERNAL MACRO CORE
========================================
*/

#if DEBUG_MASTER

    #define DEBUG_PRINT(flag, ...) \
        do { if (flag) printf(__VA_ARGS__); } while (0)

#else

    #define DEBUG_PRINT(flag, ...) do {} while (0)

#endif

/*
========================================
        MODULE-SPECIFIC LOGS
========================================
*/

#define LOG_CPU(...)   DEBUG_PRINT(DEBUG_CPU, __VA_ARGS__)
#define LOG_CB(...)    DEBUG_PRINT(DEBUG_CB, __VA_ARGS__)
#define LOG_MMU(...)   DEBUG_PRINT(DEBUG_MMU, __VA_ARGS__)
#define LOG_PPU(...)   DEBUG_PRINT(DEBUG_PPU, __VA_ARGS__)
#define LOG_TIMER(...) DEBUG_PRINT(DEBUG_TIMER, __VA_ARGS__)

/*
========================================
        COMMON DEBUG UTILS
========================================
*/

// Full CPU state dump (reuse anywhere)
#define LOG_CPU_STATE(pc, opcode, cpu) \
    LOG_CPU("[PC=0x%04X] Opcode 0x%02X | A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X SP=%04X\n", \
        pc, opcode, cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP)

// CB-prefixed version
#define LOG_CB_STATE(pc, opcode, cpu) \
    LOG_CB("[PC=0x%04X] Opcode 0xCB %02X | A=%02X F=%02X B=%02X C=%02X D=%02X E=%02X H=%02X L=%02X SP=%04X\n", \
        pc, opcode, cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP)

/*
========================================
        ERROR / ALWAYS-ON LOGS
========================================
*/

#define LOG_ERROR(...) \
    do { fprintf(stderr, __VA_ARGS__); } while (0)
