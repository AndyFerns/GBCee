#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "alu.h" // for push16

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