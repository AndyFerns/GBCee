#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// =============================================================================
// Type Definitions and Constants
// =============================================================================

// --- CHANGE: Grouped constants for better organization. ---
// Memory region sizes
#define VRAM_SIZE       0x2000 // 8KB
#define WRAM_SIZE       0x2000 // 8KB
#define OAM_SIZE        0x00A0 // 160 bytes
#define IO_SIZE         0x0080 // 128 bytes
#define HRAM_SIZE       0x007F // 127 bytes
#define MAX_ROM_SIZE    (2 * 1024 * 1024) // Max supported ROM size (2MB)
#define MAX_ERAM_SIZE   (32 * 1024)       // Max external RAM size (32KB)

// Memory region start addresses
#define ROM_START       0x0000
#define VRAM_START      0x8000
#define ERAM_START      0xA000
#define WRAM_START      0xC000
#define ECHO_RAM_START  0xE000
#define OAM_START       0xFE00
#define UNUSABLE_START  0xFEA0
#define IO_START        0xFF00
#define HRAM_START      0xFF80
#define IE_REG          0xFFFF
#define IF_REG          0xFF0F


// --- Encapsulated all MMU state into a single struct. ---
// This is a significant improvement over using multiple global variables.
// It keeps the MMU state organized, prevents polluting the global namespace,
// and makes it easier to pass the entire memory map around your emulator.
typedef struct {
    // --- CHANGE: ROM is now a pointer, not a fixed-size global array. ---
    // This allows you to dynamically allocate memory for the specific ROM
    // you are loading, rather than reserving a huge 2MB chunk of memory
    // every time. This addresses your "DONT KEEP STATIC" comment.
    uint8_t* rom;

    // Internal memory regions
    uint8_t vram[VRAM_SIZE];
    uint8_t eram[MAX_ERAM_SIZE];
    uint8_t wram[WRAM_SIZE];
    uint8_t oam[OAM_SIZE];
    uint8_t io[IO_SIZE];
    uint8_t hram[HRAM_SIZE];
    
    // Interrupt registers
    uint8_t interrupt_enable; // IE Register at 0xFFFF
    uint8_t interrupt_flag;   // IF Register at 0xFF0F

    // TODO: Add MBC (Memory Bank Controller) state variables here
    // For example: int current_rom_bank; int eram_enabled; etc.

} mmu_t;

// --- CHANGE: Create a single static instance of our MMU struct. ---
// This will hold the entire state of the Game Boy's memory.
static mmu_t mmu;


// =============================================================================
// Public Function Prototypes
// =============================================================================

void mmu_init();
void mmu_free();
int mmu_load_rom(const char* filepath);
uint8_t mmu_read(uint16_t addr);
void mmu_write(uint16_t addr, uint8_t value);


// =============================================================================
// Function Implementations
// =============================================================================

/**
 * @brief Initializes the MMU and clears all memory regions to zero.
 */
void mmu_init() {
    // --- REFACTOR: Use memset on the struct members. ---
    memset(mmu.vram, 0, sizeof(mmu.vram));
    memset(mmu.eram, 0, sizeof(mmu.eram));
    memset(mmu.wram, 0, sizeof(mmu.wram));
    memset(mmu.oam,  0, sizeof(mmu.oam));
    memset(mmu.hram, 0, sizeof(mmu.hram));
    memset(mmu.io,   0, sizeof(mmu.io));
    mmu.interrupt_enable = 0;
    mmu.interrupt_flag = 0;
    mmu.rom = NULL; // Initialize rom pointer to NULL.
    
    printf("MMU initialized.\n");
}

/**
 * @brief Frees memory allocated for the ROM.
 */
void mmu_free() {
    // --- NEW: Added a cleanup function. ---
    // It's good practice to have a function to free any dynamically
    // allocated memory when the emulator shuts down.
    if (mmu.rom != NULL) {
        free(mmu.rom);
        mmu.rom = NULL;
        printf("ROM memory freed.\n");
    }
}

/**
 * @brief Loads a ROM file into dynamically allocated memory.
 * @param filepath Path to the ROM file.
 * @return 0 on success, -1 on failure.
 */
int mmu_load_rom(const char* filepath) {
    // --- NEW: Function to handle loading the ROM file. ---
    // This separates the logic of file I/O from the MMU's core
    // read/write functionality.
    
    if (mmu.rom != NULL) {
        free(mmu.rom); // Free any previously loaded ROM
    }

    FILE* file = fopen(filepath, "rb");
    if (!file) {
        perror("Error opening ROM file");
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size > MAX_ROM_SIZE) {
        fprintf(stderr, "ROM size (%ld bytes) exceeds max supported size (%d bytes).\n", size, MAX_ROM_SIZE);
        fclose(file);
        return -1;
    }

    mmu.rom = (uint8_t*)malloc(size);
    if (!mmu.rom) {
        fprintf(stderr, "Failed to allocate memory for ROM.\n");
        fclose(file);
        return -1;
    }

    if (fread(mmu.rom, 1, size, file) != size) {
        fprintf(stderr, "Error reading ROM file.\n");
        fclose(file);
        free(mmu.rom);
        mmu.rom = NULL;
        return -1;
    }

    fclose(file);
    printf("Successfully loaded ROM: %s (%ld bytes)\n", filepath, size);
    return 0;
}


/**
 * @brief mmu_read - 
 * Reads a byte from the specified address.
 * 
 * @param addr 16-bit memory address.
 *
 * @returns Value at address.
 */
uint8_t mmu_read(uint16_t addr) {
    // --- REFACTOR: Use a switch statement for the main memory regions. ---
    // This can be slightly more efficient and is often cleaner than a long
    // if-else-if chain for handling distinct memory blocks.
    switch (addr & 0xF000) {
        // ROM Banks (0x0000 - 0x7FFF)
        case 0x0000:
        case 0x1000:
        case 0x2000:
        case 0x3000: // ROM Bank 00
            // TODO: This should always access the first 16KB of the ROM file.
            return mmu.rom[addr];
        case 0x4000:
        case 0x5000:
        case 0x6000:
        case 0x7000: // Switchable ROM Bank
            // TODO: Implement MBC logic to select the correct ROM bank.
            // For now, it just reads from the address as if it were a flat ROM.
            return mmu.rom[addr];

        // VRAM (0x8000 - 0x9FFF)
        case 0x8000:
        case 0x9000:
            return mmu.vram[addr - VRAM_START];

        // External RAM (0xA000 - 0xBFFF)
        case 0xA000:
        case 0xB000:
            // TODO: Implement MBC logic to enable/disable ERAM access.
            return mmu.eram[addr - ERAM_START];

        // Work RAM (0xC000 - 0xDFFF)
        case 0xC000:
        case 0xD000:
            return mmu.wram[addr - WRAM_START];

        // Echo RAM (0xE000 - 0xFDFF) - Mirror of 0xC000-0xDDFF
        case 0xE000:
        case 0xF000:
            if (addr <= 0xFDFF) {
                // Your original logic was correct. This mirrors the first 0x1E00 bytes of WRAM.
                return mmu.wram[addr - ECHO_RAM_START];
            }
            // --- The 0xF000 range is dense, so we handle it below. ---
            else if (addr >= OAM_START && addr <= 0xFE9F) {
                return mmu.oam[addr - OAM_START];
            }
            // --- FIX: Added handling for the unusable memory area. ---
            // Reading from 0xFEA0-0xFEFF on real hardware returns unpredictable
            // values, but returning 0xFF is a common and safe emulation practice.
            else if (addr >= UNUSABLE_START && addr <= 0xFEFF) {
                return 0xFF;
            }
            else if (addr >= IO_START && addr <= 0xFF7F) {
                // --- REFACTOR: Handle special registers inside the main block. ---
                if (addr == IF_REG) {
                    return mmu.interrupt_flag;
                }
                return mmu.io[addr - IO_START];
            }
            else if (addr >= HRAM_START && addr <= 0xFFFE) {
                return mmu.hram[addr - HRAM_START];
            }
            else if (addr == IE_REG) {
                return mmu.interrupt_enable;
            }
    }

    // --- Fallback for any unhandled addresses ---
    // Your original code correctly returned 0xFF for unmapped memory.
    return 0xFF;
}


/**
 * @brief mmu_write - 
 * Writes a byte to the specified address.
 * 
 * @param addr: 16-bit memory address.
 * @param value: Byte to write.
 * 
 * @returns void
 */
void mmu_write(uint16_t addr, uint8_t value) {
    switch (addr & 0xF000) {
        // ROM area (0x0000 - 0x7FFF) - Writes here control the MBC
        case 0x0000:
        case 0x1000: // ERAM Enable (e.g., MBC1)
        case 0x2000:
        case 0x3000: // ROM Bank Select (e.g., MBC1)
        case 0x4000:
        case 0x5000: // ROM/RAM Bank Select (e.g., MBC1)
        case 0x6000:
        case 0x7000: // Banking Mode Select (e.g., MBC1)
            // --- CHANGE: This is where MBC logic goes. ---
            // Writing to ROM is impossible, but these address ranges are used to
            // configure the memory bank controller. need to decode the
            // address and value to handle bank switching.
            // handle_mbc_write(addr, value);
            return; // We don't write to the ROM itself.

        // VRAM (0x8000 - 0x9FFF)
        case 0x8000:
        case 0x9000:
            mmu.vram[addr - VRAM_START] = value;
            return;

        // External RAM (0xA000 - 0xBFFF)
        case 0xA000:
        case 0xB000:
            // TODO: Add check here: if (eram_enabled) { ... }
            mmu.eram[addr - ERAM_START] = value;
            return;

        // Work RAM (0xC000 - 0xDFFF)
        case 0xC000:
        case 0xD000:
            mmu.wram[addr - WRAM_START] = value;
            return;

        // Echo RAM (0xE000 - 0xFDFF)
        case 0xE000:
        case 0xF000:
            if (addr <= 0xFDFF) {
                mmu.wram[addr - ECHO_RAM_START] = value;
                return;
            }
            // --- Handle the dense 0xF000 range ---
            else if (addr >= OAM_START && addr <= 0xFE9F) {
                mmu.oam[addr - OAM_START] = value;
                return;
            }
            // --- FIX: Writes to unusable memory are ignored. ---
            else if (addr >= UNUSABLE_START && addr <= 0xFEFF) {
                return;
            }
            else if (addr >= IO_START && addr <= 0xFF7F) {
                if (addr == IF_REG) {
                    mmu.interrupt_flag = value;
                    return;
                }
                // TODO: Some IO registers have special behaviors on write
                // (e.g., writing to 0xFF04-DIV resets it to 0).
                mmu.io[addr - IO_START] = value;
                return;
            }
            else if (addr >= HRAM_START && addr <= 0xFFFE) {
                mmu.hram[addr - HRAM_START] = value;
                return;
            }
            else if (addr == IE_REG) {
                mmu.interrupt_enable = value;
                return;
            }
    }
}
