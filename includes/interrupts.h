#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "alu.h" // for push16

/**
 * @brief function to service a specific interrupt
 * 
 * disables global interrupts when an interrupt is serviced 
 * 
 * corresponding bit in the IF register is cleared
 * 
 * current PC is pushed onto the stack
 * 
 * cpu jumps to the specific interrupt vector address
 * 
 * @param interrupt_bit the corresponding interrupt bit  
 * 
 * @note static defined 
 * 
 * @returns void 
 */
static void service_interrupts(int interrupt_bit);

/**
 * @brief Handles the CPU interrupt cycle
 * 
 * @details This function should be called once per cpu cycle 
 * after all hardware (cpu, ppu, timer) has been updated for the 
 * current set of cycles
 * 
 * checks for pending and enabled interrupts
 * 
 * @param none
 * 
 * @returns void
 */ 
void handle_interrupts();


#endif