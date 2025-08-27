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

TEST_CASE(ld_8bit_ops) {
    setup_test();
    run_opcode_d8(0x06, 0xAB); // LD B, 0xAB
    ASSERT_EQ(cpu.B, 0xAB, "LD B, n");
    ASSERT_EQ(cpu.PC, 0x0102, "PC advances by 2");
    
    cpu.C = 0xBE;
    cpu.PC = 0x0100;
    run_opcode(0x41); // LD B, C
    ASSERT_EQ(cpu.B, 0xBE, "LD B, C");
    ASSERT_EQ(cpu.PC, 0x0101, "PC advances by 1");
    teardown_test();
}

TEST_CASE(ld_16bit_ops) {
    setup_test();
    run_opcode_d16(0x21, 0xDEAD); // LD HL, 0xDEAD
    ASSERT_EQ(REG_HL, 0xDEAD, "LD HL, 0xDEAD");
    ASSERT_EQ(cpu.PC, 0x0103, "PC advances by 3");
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

    SET_REG_DE(0x0000);
    run_opcode(0xD1); // POP DE
    ASSERT_EQ(REG_DE, 0xABCD, "POP DE retrieves correct value");
    ASSERT_EQ(cpu.SP, 0xFFFE, "SP increments by 2 after POP");
    teardown_test();
}

TEST_CASE(add_sub_flags) {
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
    teardown_test();
}

TEST_CASE(and_or_xor_cp_flags) {
    setup_test();
    cpu.A = 0b11001100;
    run_opcode_d8(0xE6, 0b10101010); // AND 0b10101010
    ASSERT_EQ(cpu.A, 0b10001000, "AND result");
    ASSERT_EQ(cpu.F, FLAG_H, "AND should set H flag");

    cpu.A = 0b11001100;
    cpu.PC = 0x0100;
    run_opcode_d8(0xF6, 0b00110011); // OR 0b00110011
    ASSERT_EQ(cpu.A, 0b11111111, "OR result");
    ASSERT_EQ(cpu.F, 0, "OR should clear all flags");

    cpu.A = 0xFF;
    cpu.PC = 0x0100;
    run_opcode_d8(0xEE, 0xFF); // XOR 0xFF
    ASSERT_EQ(cpu.A, 0x00, "XOR result");
    ASSERT_EQ(cpu.F, FLAG_Z, "XOR should set Z flag");

    cpu.A = 0x3C;
    cpu.PC = 0x0100;
    run_opcode_d8(0xFE, 0x3C); // CP 0x3C
    ASSERT_EQ(cpu.A, 0x3C, "CP should not change A");
    ASSERT_EQ(cpu.F, FLAG_Z | FLAG_N, "CP should set Z and N flags on equal");
    teardown_test();
}

TEST_CASE(inc_dec_16bit_edge_cases) {
    setup_test();
    SET_REG_HL(0xFFFF);
    run_opcode(0x23); // INC HL
    ASSERT_EQ(REG_HL, 0x0000, "INC HL should wrap from 0xFFFF to 0x0000");
    
    SET_REG_BC(0x0000);
    cpu.PC = 0x0100;
    run_opcode(0x0B); // DEC BC
    ASSERT_EQ(REG_BC, 0xFFFF, "DEC BC should wrap from 0x0000 to 0xFFFF");
    teardown_test();
}

TEST_CASE(jumps_and_calls) {
    setup_test();
    run_opcode_d16(0xC3, 0xDEAD); // JP 0xDEAD
    ASSERT_EQ(cpu.PC, 0xDEAD, "JP should set PC to the new address");

    cpu.PC = 0x0100;
    cpu.F = FLAG_C; // Set Carry flag
    run_opcode_d16(0xD2, 0xBEEF); // JP NC, 0xBEEF (not taken)
    ASSERT_EQ(cpu.PC, 0x0103, "JP NC should not be taken when C is set");

    cpu.PC = 0x0100;
    cpu.SP = 0xFFFE;
    run_opcode_d16(0xCD, 0xFACE); // CALL 0xFACE
    ASSERT_EQ(cpu.PC, 0xFACE, "CALL should jump to new address");
    ASSERT_EQ(cpu.SP, 0xFFFC, "CALL should push return address, decrementing SP");
    ASSERT_EQ(mmu_read(0xFFFD), 0x01, "Return address high byte pushed to stack");
    ASSERT_EQ(mmu_read(0xFFFC), 0x03, "Return address low byte pushed to stack");
    teardown_test();
}

TEST_CASE(cb_bit_ops) {
    setup_test();
    cpu.A = 0b10101010;
    run_opcode_d8(0xCB, 0x7F); // BIT 7, A
    ASSERT_EQ((cpu.F & FLAG_Z), 0, "BIT 7, A should clear Z flag (bit is set)");

    cpu.A = 0b01111111;
    cpu.PC = 0x0100;
    run_opcode_d8(0xCB, 0x7F); // BIT 7, A
    ASSERT_EQ((cpu.F & FLAG_Z), FLAG_Z, "BIT 7, A should set Z flag (bit is clear)");

    cpu.B = 0b00000000;
    cpu.PC = 0x0100;
    run_opcode_d8(0xCB, 0xC0); // SET 0, B
    ASSERT_EQ(cpu.B, 0b00000001, "SET 0, B");

    cpu.C = 0b11111111;
    cpu.PC = 0x0100;
    run_opcode_d8(0xCB, 0x99); // RES 3, C
    ASSERT_EQ(cpu.C, 0b11110111, "RES 3, C");
    teardown_test();
}


// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("Starting Robust CPU test suite...\n\n");

    RUN_TEST(ld_8bit_ops);
    RUN_TEST(ld_16bit_ops);
    RUN_TEST(push_pop);
    RUN_TEST(add_sub_flags);
    RUN_TEST(and_or_xor_cp_flags);
    RUN_TEST(inc_dec_16bit_edge_cases);
    RUN_TEST(jumps_and_calls);
    RUN_TEST(cb_bit_ops);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
