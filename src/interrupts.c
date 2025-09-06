#include "interrupts.h"
#include "cpu.h"
#include "mmu.h"
#include "alu.h" // for push16()

extern CPU cpu;
extern mmu_t mmu;

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
static void service_interrupts(int interrupt_bit) {
    // disable global interrupts
    cpu.ime = false;

    // clear the corresponding bit in the Interrupt Flag register
    mmu.interrupt_flag &= ~(1 << interrupt_bit);

    // push the current Program Counter onto the stack
    push16(cpu.PC);

    // let the cpu jump to the specific interrupt vector address
    switch(interrupt_bit) {
        case 0: cpu.PC = 0x0040; break;         // V-Blank
        case 1: cpu.PC = 0x0048; break;         // LCD Stat
        case 2: cpu.PC = 0x0050; break;         // Timer
        case 3: cpu.PC = 0x0058; break;         // Serial
        case 4: cpu.PC = 0x0060; break;         // Joypad
    }
}


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
void handle_interrupts() {
    // check if the cpu is in a halted state
    if (cpu.halted && (mmu.interrupt_enable & mmu.interrupt_flag & 0x1F)) {
        cpu.halted = false;     //WAKE THE CPU UPPPPP !!!s 
    }

    // check if master IME switch is on or off 
    if (!cpu.ime) {
        return;
    }

    // check for requested and enabled interrupts (priority based)
    for (int i = 0; i < 5; i++) {
        if ((mmu.interrupt_enable & (1 << i)) && (mmu.interrupt_flag & (1 << i))) {
            service_interrupts(i);
        }
    }
}