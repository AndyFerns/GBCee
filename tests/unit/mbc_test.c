#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include "mmu.h"
#include "rom.h"

// This extern declaration allows our test file to access the global 'mmu'
// instance defined in mmu.c for verification purposes.
extern mmu_t mmu;

// =============================================================================
// A Simple Testing Framework
// =============================================================================

static int tests_run = 0;
static int tests_failed = 0;

#define TEST_CASE(name) static void test_##name()
#define RUN_TEST(name) do { printf("--- Running test: %s ---\n", #name); test_##name(); } while (0)

// Assertion macro to check for equality with clear error messages
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
// Test Helper Functions
// =============================================================================

// Helper to create a dummy ROM file on disk for testing
void create_dummy_rom(const char* filename, size_t size, mbc_type_t mbc_type) {
    FILE* f = fopen(filename, "wb");
    assert(f != NULL);

    uint8_t* buffer = (uint8_t*)malloc(size);
    assert(buffer != NULL);

    // Fill each 16KB bank with its own bank number for easy verification
    for (size_t i = 0; i < size / 0x4000; i++) {
        memset(buffer + (i * 0x4000), i, 0x4000);
    }

    // Set the MBC type code in the cartridge header (address 0x0147)
    uint8_t cart_type_code = 0;
    switch(mbc_type) {
        case MBC_TYPE_NONE: cart_type_code = 0x00; break;
        case MBC_TYPE_MBC1: cart_type_code = 0x01; break;
        case MBC_TYPE_MBC3: cart_type_code = 0x11; break;
        case MBC_TYPE_MBC5: cart_type_code = 0x19; break;
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
    create_dummy_rom(rom_name, 32 * 1024, MBC_TYPE_NONE);

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    ASSERT_EQ(mmu.mbc_type, MBC_TYPE_NONE, "ROM ONLY type detected");
    ASSERT_EQ(mmu_read(0x1234), 0x00, "Read from Bank 0");
    ASSERT_EQ(mmu_read(0x4567), 0x01, "Read from Bank 1");

    mmu_free();
    remove(rom_name);
}

TEST_CASE(mbc1_ram_enable) {
    const char* rom_name = "test_mbc1.gb";
    create_dummy_rom(rom_name, 128 * 1024, MBC_TYPE_MBC1);

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    ASSERT_EQ(mmu.ram_enabled, false, "RAM is initially disabled");
    mmu_write(0xA000, 0xAB);
    ASSERT_EQ(mmu_read(0xA000), 0xFF, "Read from disabled RAM should return 0xFF");

    mmu_write(0x0000, 0x0A);
    ASSERT_EQ(mmu.ram_enabled, true, "RAM is enabled after writing 0x0A");
    mmu_write(0xA000, 0xCD);
    ASSERT_EQ(mmu_read(0xA000), 0xCD, "Read/write to enabled RAM");

    mmu_write(0x0000, 0x00);
    ASSERT_EQ(mmu.ram_enabled, false, "RAM is disabled after writing non-0x0A value");

    mmu_free();
    remove(rom_name);
}

TEST_CASE(mbc1_rom_bank_switching_basic) {
    const char* rom_name = "test_mbc1_basic.gb";
    create_dummy_rom(rom_name, 128 * 1024, MBC_TYPE_MBC1);

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    ASSERT_EQ(mmu_read(0x1000), 0x00, "Bank 0 is always accessible");
    ASSERT_EQ(mmu_read(0x4000), 0x01, "Default switchable bank is 1");

    mmu_write(0x2100, 0x05);
    ASSERT_EQ(mmu.current_rom_bank, 5, "Switched to ROM bank 5");
    ASSERT_EQ(mmu_read(0x4000), 0x05, "Read from banked-in ROM bank 5");

    mmu_write(0x2100, 0x00);
    ASSERT_EQ(mmu.current_rom_bank, 1, "Writing 0 to bank select defaults to bank 1");
    ASSERT_EQ(mmu_read(0x4000), 0x01, "Read from default bank 1");

    mmu_free();
    remove(rom_name);
}

// --- NEW: More advanced MBC1 tests ---
TEST_CASE(mbc1_rom_bank_switching_advanced) {
    const char* rom_name = "test_mbc1_large.gb";
    create_dummy_rom(rom_name, 1024 * 1024, MBC_TYPE_MBC1); // 1MB ROM = 64 banks

    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);

    // Select bank 0x1F (31)
    mmu_write(0x2100, 0x1F);
    ASSERT_EQ(mmu_read(0x4000), 31, "Read from bank 31");

    // Now, set the upper two bits to 1 (binary 01)
    // This should select bank (0b01 << 5) | 0x1F = 0x20 | 0x1F = 0x3F (63)
    mmu_write(0x4000, 0x01);
    ASSERT_EQ(mmu.current_rom_bank, 63, "Switched to ROM bank 63 (upper bits)");
    ASSERT_EQ(mmu_read(0x4000), 63, "Read from bank 63");

    mmu_free();
    remove(rom_name);
}

// --- NEW: Placeholder tests for other MBC types ---
TEST_CASE(mbc3_detected) {
    const char* rom_name = "test_mbc3.gb";
    create_dummy_rom(rom_name, 128 * 1024, MBC_TYPE_MBC3);
    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);
    ASSERT_EQ(mmu.mbc_type, MBC_TYPE_MBC3, "MBC3 type detected");
    mmu_free();
    remove(rom_name);
}

TEST_CASE(mbc5_detected) {
    const char* rom_name = "test_mbc5.gb";
    create_dummy_rom(rom_name, 128 * 1024, MBC_TYPE_MBC5);
    mmu_init();
    assert(mmu_load_rom(rom_name) == 0);
    ASSERT_EQ(mmu.mbc_type, MBC_TYPE_MBC5, "MBC5 type detected");
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
    RUN_TEST(mbc1_rom_bank_switching_basic);
    RUN_TEST(mbc1_rom_bank_switching_advanced);
    RUN_TEST(mbc3_detected);
    RUN_TEST(mbc5_detected);

    printf("\n----------------------------------------\n");
    if (tests_failed == 0) {
        printf("All %d tests passed! ✅\n", tests_run);
    } else {
        printf("%d of %d tests failed. ❌\n", tests_failed, tests_run);
    }
    printf("----------------------------------------\n");

    return tests_failed > 0;
}
