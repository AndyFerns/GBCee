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
// We declare the main cpu and mmu structs as 'extern'. This tells the compiler
// that these variables are defined in other source files (cpu.c and mmu.c),
// and the linker will connect them. This is the correct way to access the
// internal state of a module for testing.
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
    // Create a dummy 32KB ROM space for the CPU to execute from
    mmu.rom_data = (uint8_t*)calloc(32 * 1024, 1);
    mmu.rom_size = 32 * 1024;
}

void teardown_test() {
    mmu_free();
}

// =============================================================================
// Test Cases
// =============================================================================

TEST_CASE(ld_8bit_immediate) {
    setup_test();
    mmu.rom_data[0x0100] = 0x06; // LD B, n
    mmu.rom_data[0x0101] = 0xAB;
    cpu.PC = 0x0100;
    
    cpu_step();
    
    ASSERT_EQ(cpu.B, 0xAB, "LD B, 0xAB");
    ASSERT_EQ(cpu.PC, 0x0102, "PC should advance by 2");
    teardown_test();
}

TEST_CASE(ld_8bit_reg_to_reg) {
    setup_test();
    cpu.C = 0xBE;
    mmu.rom_data[0x0100] = 0x41; // LD B, C
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.B, 0xBE, "LD B, C");
    ASSERT_EQ(cpu.PC, 0x0101, "PC should advance by 1");
    teardown_test();
}

TEST_CASE(ld_16bit_immediate) {
    setup_test();
    mmu.rom_data[0x0100] = 0x21; // LD HL, nn
    mmu.rom_data[0x0101] = 0xAD; // Low byte
    mmu.rom_data[0x0102] = 0xDE; // High byte
    cpu.PC = 0x0100;

    cpu_step();
    
    ASSERT_EQ(REG_HL, 0xDEAD, "LD HL, 0xDEAD");
    ASSERT_EQ(cpu.PC, 0x0103, "PC should advance by 3");
    teardown_test();
}

TEST_CASE(add_a_reg) {
    setup_test();
    cpu.A = 0x10;
    cpu.B = 0x05;
    mmu.rom_data[0x0100] = 0x80; // ADD A, B
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.A, 0x15, "ADD A, B result");
    ASSERT_EQ(cpu.F & FLAG_Z, 0, "Z flag should be reset");
    ASSERT_EQ(cpu.F & FLAG_N, 0, "N flag should be reset");
    ASSERT_EQ(cpu.F & FLAG_H, 0, "H flag should be reset");
    ASSERT_EQ(cpu.F & FLAG_C, 0, "C flag should be reset");
    teardown_test();
}

TEST_CASE(add_a_half_carry) {
    setup_test();
    cpu.A = 0x0F;
    cpu.B = 0x01;
    mmu.rom_data[0x0100] = 0x80; // ADD A, B
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.A, 0x10, "ADD A, B with half carry result");
    ASSERT_EQ(cpu.F & FLAG_H, FLAG_H, "H flag should be set on half carry");
    teardown_test();
}

TEST_CASE(sub_a_reg) {
    setup_test();
    cpu.A = 0x15;
    cpu.C = 0x05;
    mmu.rom_data[0x0100] = 0x91; // SUB C
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.A, 0x10, "SUB C result");
    ASSERT_EQ(cpu.F & FLAG_Z, 0, "Z flag should be reset");
    ASSERT_EQ(cpu.F & FLAG_N, FLAG_N, "N flag should be set");
    teardown_test();
}

TEST_CASE(sub_a_zero_flag) {
    setup_test();
    cpu.A = 0x15;
    cpu.C = 0x15;
    mmu.rom_data[0x0100] = 0x91; // SUB C
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.A, 0x00, "SUB C with zero result");
    ASSERT_EQ(cpu.F & FLAG_Z, FLAG_Z, "Z flag should be set on zero result");
    ASSERT_EQ(cpu.F & FLAG_N, FLAG_N, "N flag should be set");
    teardown_test();
}

TEST_CASE(inc_16bit) {
    setup_test();
    SET_REG_BC(0x1233);
    mmu.rom_data[0x0100] = 0x03; // INC BC
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(REG_BC, 0x1234, "INC BC");
    teardown_test();
}

TEST_CASE(jp_unconditional) {
    setup_test();
    mmu.rom_data[0x0100] = 0xC3; // JP nn
    mmu.rom_data[0x0101] = 0xAD; // Low byte
    mmu.rom_data[0x0102] = 0xDE; // High byte
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.PC, 0xDEAD, "JP should set PC to the new address");
    teardown_test();
}

TEST_CASE(jp_conditional_taken) {
    setup_test();
    cpu.F = FLAG_Z; // Set Z flag
    mmu.rom_data[0x0100] = 0xCA; // JP Z, nn
    mmu.rom_data[0x0101] = 0xAD; // Low byte
    mmu.rom_data[0x0102] = 0xDE; // High byte
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.PC, 0xDEAD, "Conditional JP Z should be taken when Z is set");
    teardown_test();
}

TEST_CASE(jp_conditional_not_taken) {
    setup_test();
    cpu.F = 0; // Clear Z flag
    mmu.rom_data[0x0100] = 0xCA; // JP Z, nn
    mmu.rom_data[0x0101] = 0xAD; // Low byte
    mmu.rom_data[0x0102] = 0xDE; // High byte
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ(cpu.PC, 0x0103, "Conditional JP Z should not be taken when Z is clear");
    teardown_test();
}

TEST_CASE(cb_bit_test) {
    setup_test();
    cpu.A = 0b10000000; // Bit 7 is set
    mmu.rom_data[0x0100] = 0xCB;
    mmu.rom_data[0x0101] = 0x7F; // BIT 7, A
    cpu.PC = 0x0100;

    cpu_step();

    ASSERT_EQ((cpu.F & FLAG_Z), 0, "BIT 7, A should clear Z flag when bit is set");
    ASSERT_EQ((cpu.F & FLAG_N), 0, "BIT should reset N flag");
    ASSERT_EQ((cpu.F & FLAG_H), FLAG_H, "BIT should set H flag");
    teardown_test();
}

// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("Starting CPU test suite...\n\n");

    RUN_TEST(ld_8bit_immediate);
    RUN_TEST(ld_8bit_reg_to_reg);
    RUN_TEST(ld_16bit_immediate);
    RUN_TEST(add_a_reg);
    RUN_TEST(add_a_half_carry);
    RUN_TEST(sub_a_reg);
    RUN_TEST(sub_a_zero_flag);
    RUN_TEST(inc_16bit);
    RUN_TEST(jp_unconditional);
    RUN_TEST(jp_conditional_taken);
    RUN_TEST(jp_conditional_not_taken);
    RUN_TEST(cb_bit_test);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
