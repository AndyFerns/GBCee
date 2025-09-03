#ifndef TIMER_H
#define TIMER_H

/**
 * @file timer.h
 * @brief Public interface for the Game Boy's timer and divider system.
 *
 * This module is responsible for emulating the behavior of the DIV, TIMA,
 * TMA, and TAC registers, including their interactions and the generation
 * of timer interrupts.
 */

/**
 * @brief Updates the timer system by a given number of clock cycles.
 *
 * @param cycles The number of T-cycles (the smallest time unit) that have
 * passed since the last update. 
 * 
 * This value is typically
 * provided by the CPU after executing an instruction.
 *
 * @details 
 * 
 * Increments the internal 16-bit DIV counter by the number of
 * elapsed cycles. 
 * 
 * Checks if the timer is enabled via the TAC register (bit 2).
 * 
 * If enabled, it determines the correct frequency for the TIMA
 * counter based on the lower two bits of the TAC register.
 * 
 *  It checks for a "falling edge" on a specific bit of the internal
 * DIV counter, which is the trigger for incrementing TIMA.
 * 
 * If TIMA overflows (goes from 0xFF to 0x00), it reloads TIMA with
 * the value from the TMA register and requests a timer interrupt
 * by setting bit 2 of the IF (0xFF0F) register.
 * 
 * @returns void
 */
void timer_step(int cycles);

#endif