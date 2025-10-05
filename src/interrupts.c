#include "interrupts.h"
#include "cpu.h"
#include "mmu.h"
#include "alu.h" // for push16()

extern CPU cpu;
extern mmu_t mmu;

/**
 * @brief Services a single, specific interrupt by jumping the CPU.
 * 
 * @param interrupt_bit The bit number (0-4) of the interrupt to service.
 * 
 * @details This function performs the hardware-accurate sequence for an interrupt call:
 * 1. Disables the master interrupt switch (IME).
 * 2. Clears the corresponding request bit in the IF register.
 * 3. Pushes the current Program Counter onto the stack.
 * 4. Jumps the CPU to the specific interrupt's vector address.
 * 
 * @returns void
 * 
 * @note static
 */
static void service_interrupts(int interrupt_bit) {
    // 1. When an interrupt is serviced, global interrupts are disabled immediately.
    cpu.ime = false;

    // 2. The corresponding request bit in the IF register (0xFF0F) is cleared.
    // We use the getter/setter functions to maintain encapsulation.
    uint8_t if_reg = mmu_get_if_register();
    mmu_write(0xFF0F, if_reg & ~(1 << interrupt_bit));

    // 3. The current Program Counter is pushed onto the stack.
    push16(cpu.PC);

    // 4. The CPU's Program Counter is set to the interrupt's vector address.
    switch (interrupt_bit) {
        case 0: cpu.PC = 0x0040; break; // V-Blank Interrupt
        case 1: cpu.PC = 0x0048; break; // LCD STAT Interrupt
        case 2: cpu.PC = 0x0050; break; // Timer Interrupt
        case 3: cpu.PC = 0x0058; break; // Serial Interrupt
        case 4: cpu.PC = 0x0060; break; // Joypad Interrupt
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
    // Determine which interrupts are both requested (in IF) and enabled (in IE).
    uint8_t requested_interrupts = mmu_get_if_register();
    uint8_t enabled_interrupts = mmu_get_ie_register();
    uint8_t active_interrupts = requested_interrupts & enabled_interrupts & 0x1F;

    // --- Wake from HALT ---
    // If the CPU is in a HALT state and there are any active interrupts,
    // it should wake up on the next cycle.
    if (cpu.halted && (requested_interrupts & 0x1F)) {
        cpu.halted = false;
    }

    // --- Service Interrupts ---
    // Interrupts can only be serviced if the master interrupt switch (IME) is enabled.
    if (!cpu.ime) {
        return;
    }

    // If there are active interrupts to service, handle the one with the highest priority.
    if (active_interrupts) {
        // Loop from bit 0 (V-Blank, highest priority) to bit 4 (Joypad, lowest).
        for (int i = 0; i < 5; i++) {
            if (active_interrupts & (1 << i)) {
                service_interrupts(i);
                // Only one interrupt is serviced per instruction cycle.
                break;
            }
        }
    }
}