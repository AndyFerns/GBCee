#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// --- Test Suite Setup ---
// We include the .c file directly to gain access to the static `mmu` struct.
// This is a common technique for testing static components in C.
#include "../src/mmu.c"
#include "../includes/rom.h" // NEW: Include rom.h to get the mbc_type_t enum definition

// =============================================================================
// A Simple Testing Framework
// =============================================================================

static int tests_run = 0;
static int tests_failed = 0;

#define TEST_CASE(name) static void test_##name()
#define RUN_TEST(name) do { printf("--- Running test: %s ---\n", #name); test_##name(); } while (0)

// Assertion macro to check for equality
#define ASSERT_EQ(a, b, message) \
    do { \
        tests_run++; \
        if ((a) != (b)) { \
            fprintf(stderr, "    [FAIL] %s:%d: " message " - Expected 0x%X, got 0x%X\n", __FILE__, __LINE__, (int)(a), (int)(b)); \
            tests_failed++; \
        } else { \
            printf("    [PASS] %s\n", message); \
        } \
    } while (0)

// =============================================================================
// Test Helper Functions
// =============================================================================

// Helper to create a dummy ROM file on disk for testing
void create_dummy_rom(const char* filename, size_t size, mbc_type_t mbc_type) {
    FILE* f = fopen(filename, "wb");
    assert(f != NULL);

    uint8_t* buffer = (uint8_t*)malloc(size);
    assert(buffer != NULL);

    // Fill each 16KB bank with its own bank number
    for (size_t i = 0; i < size / 0x4000; i++) {
        memset(buffer + (i * 0x4000), i, 0x4000);
    }

    // Set the MBC type in the header
    // NOTE: Your rom.c needs to map the cartridge type code (e.g., 0x00) to the enum
    uint8_t cart_type_code = 0;
    switch(mbc_type) {
        case MBC_TYPE_NONE: cart_type_code = 0x00; break;
        case MBC_TYPE_MBC1: cart_type_code = 0x01; break;
        // Add other mappings here
        default: break;
    }
    buffer[0x0147] = cart_type_code;


    fwrite(buffer, 1, size, f);
    fclose(f);
    free(buffer);
}

// =============================================================================
// Test Cases
// =============================================================================

TEST_CASE(rom_only_read) {
    const char* rom_name = "test_rom_only.gb";
    // CORRECTED: Use the name from your rom.h enum
    create_dummy_rom(rom_name, 32 * 1024, MBC_TYPE_NONE);

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    // For ROM_ONLY, the mbc_type should be correctly identified.
    ASSERT_EQ(mmu.mbc_type, MBC_TYPE_NONE, "ROM ONLY type detected");

    // Bank 0 should be readable
    ASSERT_EQ(mmu_read(0x1234), 0x00, "Read from Bank 0");
    // Bank 1 should be readable
    ASSERT_EQ(mmu_read(0x4567), 0x01, "Read from Bank 1");

    mmu_free();
    remove(rom_name);
}

TEST_CASE(mbc1_ram_enable) {
    const char* rom_name = "test_mbc1.gb";
    create_dummy_rom(rom_name, 128 * 1024, MBC_TYPE_MBC1);

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    // RAM should be disabled by default
    ASSERT_EQ(mmu.ram_enabled, false, "RAM is initially disabled");
    mmu_write(0xA000, 0xAB);
    ASSERT_EQ(mmu_read(0xA000), 0xFF, "Read from disabled RAM should return 0xFF");

    // Enable RAM by writing 0x0A to the enable register range
    mmu_write(0x0000, 0x0A);
    ASSERT_EQ(mmu.ram_enabled, true, "RAM is enabled after writing 0x0A");
    mmu_write(0xA000, 0xCD);
    ASSERT_EQ(mmu_read(0xA000), 0xCD, "Read/write to enabled RAM");

    // Disable RAM by writing any other value
    mmu_write(0x0000, 0x00);
    ASSERT_EQ(mmu.ram_enabled, false, "RAM is disabled after writing non-0x0A value");

    mmu_free();
    remove(rom_name);
}

TEST_CASE(mbc1_rom_bank_switching) {
    const char* rom_name = "test_mbc1.gb";
    // Create a 128KB ROM (8 banks of 16KB)
    create_dummy_rom(rom_name, 128 * 1024, MBC_TYPE_MBC1);

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    // Bank 0 is always at 0x0000-0x3FFF
    ASSERT_EQ(mmu_read(0x1000), 0x00, "Bank 0 is always accessible");

    // By default, bank 1 is at 0x4000-0x7FFF
    ASSERT_EQ(mmu_read(0x4000), 0x01, "Default switchable bank is 1");

    // Switch to bank 5
    mmu_write(0x2100, 0x05);
    ASSERT_EQ(mmu.current_rom_bank, 5, "Switched to ROM bank 5");
    ASSERT_EQ(mmu_read(0x4000), 0x05, "Read from banked-in ROM bank 5");

    // Switch to bank 7 (which is bank index 7)
    mmu_write(0x2100, 0x07);
    ASSERT_EQ(mmu.current_rom_bank, 7, "Switched to ROM bank 7");
    ASSERT_EQ(mmu_read(0x4000), 0x07, "Read from banked-in ROM bank 7");

    // Writing 0 to the bank select register should select bank 1 instead
    mmu_write(0x2100, 0x00);
    ASSERT_EQ(mmu.current_rom_bank, 1, "Writing 0 to bank select defaults to bank 1");
    ASSERT_EQ(mmu_read(0x4000), 0x01, "Read from default bank 1");

    mmu_free();
    remove(rom_name);
}

// =============================================================================
// Test Runner
// =============================================================================

int main() {
    printf("Starting MBC test suite...\n\n");

    RUN_TEST(rom_only_read);
    RUN_TEST(mbc1_ram_enable);
    RUN_TEST(mbc1_rom_bank_switching);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
