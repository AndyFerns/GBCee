#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mmu.h"
#include "rom.h"

// We declare the main mmu struct as 'extern' to access its internal state.
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

// =============================================================================
// Test Cases
// =============================================================================

TEST_CASE(initialization) {
    mmu_init();
    ASSERT_EQ(mmu_read(0xC000), 0x00, "WRAM should be 0 after init");
    ASSERT_EQ(mmu_read(0x8000), 0x00, "VRAM should be 0 after init");
    ASSERT_EQ(mmu_read(0xFF80), 0x00, "HRAM should be 0 after init");
    ASSERT_EQ(mmu_read(0xFFFF), 0x00, "IE register should be 0 after init");
    mmu_free();
}

TEST_CASE(wram_read_write) {
    mmu_init();
    mmu_write(0xC123, 0xAB);
    mmu_write(0xD456, 0xCD);
    ASSERT_EQ(mmu_read(0xC123), 0xAB, "WRAM read/write at 0xC123");
    ASSERT_EQ(mmu_read(0xD456), 0xCD, "WRAM read/write at 0xD456");
    mmu_free();
}

TEST_CASE(vram_read_write) {
    mmu_init();
    mmu_write(0x8000, 0xFE);
    mmu_write(0x9FFF, 0xDC);
    ASSERT_EQ(mmu_read(0x8000), 0xFE, "VRAM read/write at 0x8000");
    ASSERT_EQ(mmu_read(0x9FFF), 0xDC, "VRAM read/write at 0x9FFF");
    mmu_free();
}

TEST_CASE(echo_ram) {
    mmu_init();
    // Write to WRAM, read from Echo RAM
    mmu_write(0xC005, 0x42);
    ASSERT_EQ(mmu_read(0xE005), 0x42, "Write to WRAM should be reflected in Echo RAM");

    // Write to Echo RAM, read from WRAM
    mmu_write(0xE006, 0x88);
    ASSERT_EQ(mmu_read(0xC006), 0x88, "Write to Echo RAM should be reflected in WRAM");
    mmu_free();
}

TEST_CASE(unusable_memory) {
    mmu_init();
    // Write to the unusable area
    mmu_write(0xFEA0, 0x55);
    
    // Verify that reading from the unusable area returns 0xFF
    ASSERT_EQ(mmu_read(0xFEA5), 0xFF, "Read from unusable memory should return 0xFF");
    mmu_free();
}

// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("Starting MMU test suite...\n\n");

    RUN_TEST(initialization);
    RUN_TEST(wram_read_write);
    RUN_TEST(vram_read_write);
    RUN_TEST(echo_ram);
    RUN_TEST(unusable_memory);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
