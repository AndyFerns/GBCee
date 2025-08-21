#include <stdio.h>
#include <string.h>
#include <assert.h>

// We need to include the C file directly to gain access to the static `mmu`
// struct for our tests. This is a common technique for testing static functions
// and variables in C without modifying the original source file.
#include "../src/mmu.c"

// =============================================================================
// A Simple Testing Framework
// =============================================================================

// Global counters for test results
static int tests_run = 0;
static int tests_failed = 0;

// Macro to define a test case function
#define TEST_CASE(name) void test_##name()

// Macro to run a test case and print its name
#define RUN_TEST(name) \
    do { \
        printf("--- Running test: %s ---\n", #name); \
        test_##name(); \
    } while (0)

// Assertion macro to check for equality
#define ASSERT_EQ(a, b, message) \
    do { \
        tests_run++; \
        if ((a) != (b)) { \
            fprintf(stderr, "    [FAIL] %s:%d: " message " - Expected %d, got %d\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
            tests_failed++; \
        } else { \
            printf("    [PASS] %s\n", message); \
        } \
    } while (0)


// =============================================================================
// Test Cases
// =============================================================================

/**
 * @brief Test Case: MMU Initialization
 * Verifies that all RAM regions are zeroed out after mmu_init().
 */
TEST_CASE(initialization) {
    mmu_init();
    ASSERT_EQ(0, mmu_read(0xC000), "WRAM should be 0 after init");
    ASSERT_EQ(0, mmu_read(0x8000), "VRAM should be 0 after init");
    ASSERT_EQ(0, mmu_read(0xA000), "ERAM should be 0 after init");
    ASSERT_EQ(0, mmu_read(0xFF80), "HRAM should be 0 after init");
    ASSERT_EQ(0, mmu_read(0xFFFF), "IE register should be 0 after init");
    mmu_free();
}

/**
 * @brief Test Case: Basic Read/Write Operations
 * Verifies that we can write to and read from various RAM regions.
 */
TEST_CASE(read_write) {
    mmu_init();
    
    // Test WRAM
    mmu_write(0xC123, 0xAB);
    ASSERT_EQ(0xAB, mmu_read(0xC123), "WRAM read/write");

    // Test VRAM
    mmu_write(0x8ABC, 0xCD);
    ASSERT_EQ(0xCD, mmu_read(0x8ABC), "VRAM read/write");

    // Test HRAM
    mmu_write(0xFF85, 0xEF);
    ASSERT_EQ(0xEF, mmu_read(0xFF85), "HRAM read/write");

    // Test IE Register
    mmu_write(0xFFFF, 0x15);
    ASSERT_EQ(0x15, mmu_read(0xFFFF), "IE Register read/write");

    mmu_free();
}

/**
 * @brief Test Case: Echo RAM
 * Verifies that writes to WRAM are mirrored in Echo RAM and vice-versa.
 */
TEST_CASE(echo_ram) {
    mmu_init();

    // Write to WRAM, read from Echo RAM
    mmu_write(0xC005, 0x42);
    ASSERT_EQ(0x42, mmu_read(0xE005), "Write to WRAM should be reflected in Echo RAM");

    // Write to Echo RAM, read from WRAM
    mmu_write(0xE006, 0x88);
    ASSERT_EQ(0x88, mmu_read(0xC006), "Write to Echo RAM should be reflected in WRAM");
    
    mmu_free();
}

/**
 * @brief Test Case: Unusable Memory Area
 * Verifies the behavior of the unusable memory region (0xFEA0 - 0xFEFF).
 */
TEST_CASE(unusable_memory) {
    mmu_init();
    
    // Write a value to WRAM to ensure it's not zero
    mmu_write(0xCFFF, 0x12);

    // Write to the unusable area
    mmu_write(0xFEA0, 0x55);
    
    // Verify that the write was ignored (by checking a nearby memory location)
    ASSERT_EQ(0x12, mmu_read(0xCFFF), "Write to unusable memory should be ignored");

    // Verify that reading from the unusable area returns 0xFF
    ASSERT_EQ(0xFF, mmu_read(0xFEA5), "Read from unusable memory should return 0xFF");

    mmu_free();
}

/**
 * @brief Test Case: ROM Loading and Reading
 * Verifies that a ROM file can be loaded and its contents read correctly.
 */
TEST_CASE(rom_loading) {
    mmu_init();

    // Create a dummy ROM file for testing
    const char* rom_filename = "dummy_rom.gb";
    FILE* f = fopen(rom_filename, "wb");
    assert(f != NULL);
    uint8_t rom_data[] = {0x01, 0x02, 0x03, 0x04, 0xDE, 0xAD, 0xBE, 0xEF};
    fwrite(rom_data, 1, sizeof(rom_data), f);
    fclose(f);

    // Test loading
    int result = mmu_load_rom(rom_filename);
    ASSERT_EQ(0, result, "mmu_load_rom should return 0 on success");

    // Test reading from ROM
    ASSERT_EQ(0x01, mmu_read(0x0000), "Read first byte of ROM");
    ASSERT_EQ(0xEF, mmu_read(0x0007), "Read last byte of ROM");

    // Test writing to ROM (should be ignored)
    mmu_write(0x0002, 0xFF);
    ASSERT_EQ(0x03, mmu_read(0x0002), "Writing to ROM should have no effect");

    mmu_free(); // This will free the loaded ROM memory
    remove(rom_filename); // Clean up the dummy file
}


// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("Starting MMU test suite...\n\n");

    RUN_TEST(initialization);
    RUN_TEST(read_write);
    RUN_TEST(echo_ram);
    RUN_TEST(unusable_memory);
    RUN_TEST(rom_loading);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0; // Return 1 if any tests failed, 0 otherwise
}
