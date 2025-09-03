#include "timer.h"
#include "mmu.h"
#include "cpu.h"

// Allow this file to access the global mmu and cpu state
extern mmu_t mmu;
extern CPU cpu;

// The timer interrupt is on bit 2 of the IF register
#define TIMER_INTERRUPT_BIT 2

/**
 * @brief 
 * 
 * @param cycles :number of CPU cycles
 * 
 * @returns void
 */
void timer_step(int cycles) {
    // 1. Handle the DIV register
    // The internal 16-bit counter increments every 4 T-cycles.
    // Since our `cycles` are already T-cycles, we just add them.
    uint16_t old_timer = mmu.internal_timer;
    mmu.internal_timer += cycles;

    // Checks if the timer is enabled in the TAC register
    bool timer_enabled = (mmu.tac & 0x04) != 0;
    if (!timer_enabled) {
        return;
    }

    // 3. Determine which bit of the internal counter to check for TIMA increment
    // This is the tricky part that makes the timer cycle-accurate.
    // TIMA increments on a "falling edge" of a specific bit in the internal counter.
    int bit_to_check = 0;
    switch (mmu.tac & 0x03) {
        case 0: bit_to_check = 9; break;  // 4096 Hz
        case 1: bit_to_check = 3; break;  // 262144 Hz
        case 2: bit_to_check = 5; break;  // 65536 Hz
        case 3: bit_to_check = 7; break;  // 16384 Hz
    }

    // Check for the falling edge: was the bit 1 before, and is it 0 now?
    bool was_set = (old_timer >> bit_to_check) & 1;
    bool is_set = (mmu.internal_timer >> bit_to_check) & 1;

    if (was_set && !is_set) {
        // Falling edge detected! Increment TIMA.
        mmu.tima++;
        if (mmu.tima == 0) { // Check for overflow (0xFF -> 0x00)
            // On overflow, reload TIMA with the value from TMA
            mmu.tima = mmu.tma;
            
            // And request a timer interrupt
            mmu.interrupt_flag |= (1 << TIMER_INTERRUPT_BIT);
        }
    }
}