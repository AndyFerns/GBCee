#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cpu.h"
#include "mmu.h"
#include "rom.h"
#include "alu.h"

// --- Test Suite Setup ---
extern CPU cpu;
extern mmu_t mmu;

// =============================================================================
// A Simple Testing Framework
// =============================================================================

static int tests_run = 0;
static int tests_failed = 0;

#define TEST_CASE(name) static void test_##name()
#define RUN_TEST(name) do { printf("--- Running test: %s ---\n", #name); test_##name(); } while (0)

#define ASSERT_EQ(a, b, message) \
    do { \
        tests_run++; \
        if ((a) != (b)) { \
            fprintf(stderr, "    [FAIL] %s:%d: " message " - Expected 0x%X, got 0x%X\n", __FILE__, __LINE__, (int)(b), (int)(a)); \
            tests_failed++; \
        } else { \
            printf("    [PASS] %s\n", message); \
        } \
    } while (0)

// Helper to reset CPU and MMU state before each test
void setup_test() {
    mmu_init();
    cpu_reset();
    mmu.rom_data = (uint8_t*)calloc(32 * 1024, 1);
    mmu.rom_size = 32 * 1024;
    cpu.PC = 0x0100; // Default start for all tests
}

void teardown_test() {
    mmu_free();
}

// Helper to execute a single opcode placed at 0x0100
void run_opcode(uint8_t opcode) {
    mmu.rom_data[0x0100] = opcode;
    cpu_step();
}

// Helper for 2-byte opcodes
void run_opcode_d8(uint8_t opcode, uint8_t d8) {
    mmu.rom_data[0x0100] = opcode;
    mmu.rom_data[0x0101] = d8;
    cpu_step();
}

// Helper for 3-byte opcodes
void run_opcode_d16(uint8_t opcode, uint16_t d16) {
    mmu.rom_data[0x0100] = opcode;
    mmu.rom_data[0x0101] = d16 & 0xFF;
    mmu.rom_data[0x0102] = (d16 >> 8) & 0xFF;
    cpu_step();
}

// =============================================================================
// Test Cases
// =============================================================================

TEST_CASE(ld_8bit_all) {
    setup_test();
    run_opcode_d8(0x06, 0xAB); // LD B, n
    ASSERT_EQ(cpu.B, 0xAB, "LD B, n");
    
    cpu.C = 0xBE;
    cpu.PC = 0x0100;
    run_opcode(0x41); // LD B, C
    ASSERT_EQ(cpu.B, 0xBE, "LD B, C");

    SET_REG_HL(0xC000);
    mmu_write(0xC000, 0xFE);
    cpu.PC = 0x0100;
    run_opcode(0x46); // LD B, (HL)
    ASSERT_EQ(cpu.B, 0xFE, "LD B, (HL)");

    cpu.A = 0xFA;
    cpu.PC = 0x0100;
    run_opcode(0x47); // LD B, A
    ASSERT_EQ(cpu.B, 0xFA, "LD B, A");
    teardown_test();
}

TEST_CASE(ld_16bit_all) {
    setup_test();
    run_opcode_d16(0x01, 0xBEEF); // LD BC, nn
    ASSERT_EQ(REG_BC, 0xBEEF, "LD BC, nn");

    cpu.SP = 0x1234;
    cpu.PC = 0x0100;
    run_opcode_d16(0x08, 0xC000); // LD (nn), SP
    ASSERT_EQ(mmu_read(0xC000), 0x34, "LD (nn), SP low byte");
    ASSERT_EQ(mmu_read(0xC001), 0x12, "LD (nn), SP high byte");
    
    SET_REG_HL(0xABCD);
    cpu.PC = 0x0100;
    run_opcode(0xF9); // LD SP, HL
    ASSERT_EQ(cpu.SP, 0xABCD, "LD SP, HL");
    teardown_test();
}

TEST_CASE(push_pop) {
    setup_test();
    SET_REG_BC(0xABCD);
    cpu.SP = 0xFFFE;
    run_opcode(0xC5); // PUSH BC
    ASSERT_EQ(cpu.SP, 0xFFFC, "SP decrements by 2 after PUSH");
    ASSERT_EQ(mmu_read(0xFFFD), 0xAB, "PUSH writes high byte");
    ASSERT_EQ(mmu_read(0xFFFC), 0xCD, "PUSH writes low byte");

    cpu.PC = 0x0100;
    SET_REG_DE(0x0000);
    run_opcode(0xD1); // POP DE
    ASSERT_EQ(REG_DE, 0xABCD, "POP DE retrieves correct value");
    ASSERT_EQ(cpu.SP, 0xFFFE, "SP increments by 2 after POP");
    teardown_test();
}

TEST_CASE(alu_8bit_flags) {
    setup_test();
    cpu.A = 0x0F;
    cpu.B = 0x01;
    run_opcode(0x80); // ADD A, B
    ASSERT_EQ(cpu.A, 0x10, "ADD A, B result");
    ASSERT_EQ(cpu.F, FLAG_H, "ADD should set Half Carry flag");

    cpu.A = 0xFF;
    cpu.B = 0x01;
    cpu.PC = 0x0100;
    run_opcode(0x80); // ADD A, B
    ASSERT_EQ(cpu.A, 0x00, "ADD with carry result");
    ASSERT_EQ(cpu.F, FLAG_Z | FLAG_H | FLAG_C, "ADD should set Z, H, and C flags");

    cpu.A = 0x10;
    cpu.C = 0x01;
    cpu.PC = 0x0100;
    run_opcode(0x91); // SUB C
    ASSERT_EQ(cpu.A, 0x0F, "SUB result");
    ASSERT_EQ(cpu.F, FLAG_N | FLAG_H, "SUB should set N and H flags");
    
    cpu.A = 0x3C;
    cpu.PC = 0x0100;
    run_opcode_d8(0xFE, 0x40); // CP 0x40
    ASSERT_EQ(cpu.F, FLAG_N | FLAG_C, "CP should set N and C when A < n");
    teardown_test();
}

TEST_CASE(alu_16bit_flags) {
    setup_test();
    // Z flag is preserved, so we test both initial states
    cpu.F = 0; // Z flag is initially clear
    SET_REG_HL(0x0FFF);
    SET_REG_BC(0x0001);
    run_opcode(0x09); // ADD HL, BC
    ASSERT_EQ(REG_HL, 0x1000, "ADD HL, BC result");
    ASSERT_EQ(cpu.F, FLAG_H, "ADD HL should set H flag (Z clear)");

    cpu.F = FLAG_Z; // Z flag is initially set
    SET_REG_HL(0xFFFF);
    SET_REG_BC(0x0001);
    cpu.PC = 0x0100;
    run_opcode(0x09); // ADD HL, BC
    ASSERT_EQ(REG_HL, 0x0000, "ADD HL, BC with overflow result");
    // CORRECTED: Z flag is preserved, not reset
    ASSERT_EQ(cpu.F, FLAG_Z | FLAG_H | FLAG_C, "ADD HL should set H and C, and preserve Z");
    teardown_test();
}

TEST_CASE(misc_ops) {
    setup_test();
    cpu.A = 0x19;
    cpu.F = 0; // Flags clear from previous ADD
    run_opcode(0x27); // DAA
    ASSERT_EQ(cpu.A, 0x19, "DAA on 0x19 (no change)");

    cpu.A = 0x3A;
    cpu.F = 0;
    cpu.PC = 0x0100;
    run_opcode(0x27); // DAA
    ASSERT_EQ(cpu.A, 0x40, "DAA on 0x3A should correct to 0x40");

    cpu.A = 0xAB;
    cpu.PC = 0x0100;
    run_opcode(0x2F); // CPL
    ASSERT_EQ(cpu.A, 0x54, "CPL should invert bits");
    ASSERT_EQ(cpu.F, FLAG_N | FLAG_H, "CPL should set N and H flags");
    teardown_test();
}

TEST_CASE(rotates_and_shifts) {
    setup_test();
    cpu.A = 0b10000001;
    run_opcode(0x07); // RLCA
    ASSERT_EQ(cpu.A, 0b00000011, "RLCA result");
    ASSERT_EQ(cpu.F, FLAG_C, "RLCA should set C flag");

    cpu.A = 0b10000001;
    cpu.F = FLAG_C;
    cpu.PC = 0x0100;
    run_opcode(0x17); // RLA
    ASSERT_EQ(cpu.A, 0b00000011, "RLA result");
    ASSERT_EQ(cpu.F, FLAG_C, "RLA should set C flag from old bit 7");
    
    cpu.A = 0b10000001;
    cpu.PC = 0x0100;
    run_opcode(0x0F); // RRCA
    ASSERT_EQ(cpu.A, 0b11000000, "RRCA result");
    ASSERT_EQ(cpu.F, FLAG_C, "RRCA should set C flag");
    teardown_test();
}

TEST_CASE(jumps_and_calls) {
    setup_test();
    run_opcode_d16(0xC3, 0xDEAD); // JP 0xDEAD
    ASSERT_EQ(cpu.PC, 0xDEAD, "JP should set PC to the new address");

    cpu.PC = 0x0100;
    run_opcode_d8(0x18, 0x05); // JR 5
    ASSERT_EQ(cpu.PC, 0x0107, "JR should jump relative to next instruction");
    
    cpu.PC = 0x0100;
    run_opcode_d8(0x18, 0xFA); // JR -6
    ASSERT_EQ(cpu.PC, 0x00FC, "JR should handle negative offsets");

    cpu.PC = 0x0100;
    cpu.SP = 0xFFFE;
    run_opcode_d16(0xCD, 0xFACE); // CALL 0xFACE
    ASSERT_EQ(cpu.PC, 0xFACE, "CALL should jump to new address");
    ASSERT_EQ(cpu.SP, 0xFFFC, "CALL should push return address");
    ASSERT_EQ(mmu_read(0xFFFD), 0x01, "Return address high byte");
    ASSERT_EQ(mmu_read(0xFFFC), 0x03, "Return address low byte");
    teardown_test();
}

TEST_CASE(returns) {
    setup_test();
    cpu.SP = 0xFFFC;
    mmu_write(0xFFFD, 0xBE);
    mmu_write(0xFFFC, 0xEF);
    run_opcode(0xC9); // RET
    ASSERT_EQ(cpu.PC, 0xBEEF, "RET should pop PC from stack");
    ASSERT_EQ(cpu.SP, 0xFFFE, "RET should increment SP");

    cpu.SP = 0xFFFC;
    cpu.PC = 0x0100;
    cpu.F = FLAG_Z;
    run_opcode(0xC0); // RET NZ (not taken)
    ASSERT_EQ(cpu.PC, 0x0101, "RET NZ should not be taken when Z is set");
    ASSERT_EQ(cpu.SP, 0xFFFC, "SP should not change on untaken RET");
    teardown_test();
}

TEST_CASE(cb_all_ops) {
    setup_test();
    cpu.A = 0b10000001;
    run_opcode_d8(0xCB, 0x07); // RLC A
    ASSERT_EQ(cpu.A, 0b00000011, "RLC A result");
    ASSERT_EQ(cpu.F, FLAG_C, "RLC A should set C flag");

    cpu.B = 0b10000000;
    cpu.PC = 0x0100;
    run_opcode_d8(0xCB, 0x20); // SLA B
    ASSERT_EQ(cpu.B, 0x00, "SLA B result");
    ASSERT_EQ(cpu.F, FLAG_Z | FLAG_C, "SLA B should set Z and C flags");
    
    cpu.C = 0b00000001;
    cpu.PC = 0x0100;
    run_opcode_d8(0xCB, 0x29); // SRA C
    ASSERT_EQ(cpu.C, 0x00, "SRA C result");
    ASSERT_EQ(cpu.F, FLAG_Z | FLAG_C, "SRA C should set Z and C flags");

    cpu.D = 0b11111111;
    cpu.PC = 0x0100;
    run_opcode_d8(0xCB, 0x3A); // SRL D
    ASSERT_EQ(cpu.D, 0b01111111, "SRL D result");
    ASSERT_EQ(cpu.F, FLAG_C, "SRL D should set C flag");
    teardown_test();
}

// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("Starting Exhaustive CPU test suite...\n\n");

    RUN_TEST(ld_8bit_all);
    RUN_TEST(ld_16bit_all);
    RUN_TEST(push_pop);
    RUN_TEST(alu_8bit_flags);
    RUN_TEST(alu_16bit_flags);
    RUN_TEST(misc_ops);
    RUN_TEST(rotates_and_shifts);
    RUN_TEST(jumps_and_calls);
    RUN_TEST(returns);
    RUN_TEST(cb_all_ops);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
