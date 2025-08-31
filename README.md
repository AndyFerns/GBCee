# GBCee (A gameboy emulator in C)

GBCee is a work-in-progress Gameboy (DMG-01) emulator written from scratch in C.
This project is a deep dive into low-level systems programming, computer architecture, and the intricacies of early handheld gaming hardware.
The primary goal is to create a cycle-accurate and well-documented emulator for fun and learning.

## Current State: Debug Output

While the PPU (graphics) is still under development, the CPU core is fully implemented and correctly executes the instruction set of many games. The emulator currently outputs a detailed log of the CPU's state for each instruction, which is invaluable for debugging and verification.

```powershell
# Example of the current debug output
AF: 01B0  BC: 0013  DE: 00D8  HL: 014D  SP: FFFE  PC: 0100 | Opcode: 00 (NOP)
AF: 01B0  BC: 0013  DE: 00D8  HL: 014D  SP: FFFE  PC: 0101 | Opcode: 21 (LD HL,d16) | Operand: 014D
AF: 01B0  BC: 0013  DE: 00D8  HL: 014D  SP: FFFC  PC: 0104 | Opcode: 32 (LD (HL-),A)
AF: F1B0  BC: 0013  DE: 00D8  HL: 014C  SP: FFFC  PC: 0105 | Opcode: CB 7C (BIT 7,H)
...
```

---

## ✅ Implemented Features

The emulator has a solid foundation with several core components already in place.

* **Complete CPU Core:**
  * Full implementation of the ~500 standard and CB-prefixed Gameboy opcodes.
  * Correctly manages all 8-bit and 16-bit registers.
  * Handles the fetch-decode-execute cycle with proper program counter management.

* **Memory Management Unit (MMU):**
  * Maps the entire Gameboy memory layout (VRAM, WRAM, OAM, IO, etc.).
  * Abstracted memory access via `mmu_read()` and `mmu_write()` functions.

* **Cartridge Support:**
  * Loads `.gb` ROM files directly into memory.
  * **MBC0** (No banking) support for simple games like Tetris.
  * **MBC1** support, enabling bank switching for more complex games.

* **Debugging & Display:**
  * Real-time disassembly and register logging to the console.
  * Basic window scaffolding using the **SDL2** library, ready for PPU integration.

---

## 🗺️ Project Roadmap

This is the roadmap to a fully functional emulator. Items will be checked off as they are implemented.

### Graphics (PPU)

* [ ] Implement the PPU state machine (OAM Scan, Drawing, HBlank, VBlank).
* [ ] Render background and window tiles from VRAM.
* [ ] Render sprites (OAM).
* [ ] Draw the final 160x144 pixel buffer to the SDL window.
* [ ] Handle VBlank interrupts correctly.

### Timers & Interrupts

* [ ] Implement DIV, TIMA, TMA, and TAC timer registers.
* [ ] Implement the Interrupt Master Enable (IME) flag and handler.
* [ ] Fire interrupts correctly for VBlank, LCD STAT, Timer, etc.

### Input Handling

* [ ] Map keyboard keys to the Gameboy's button inputs (A, B, Start, Select, D-Pad).
* [ ] Write button press states to the JOYP register (0xFF00).

### Advanced Features

* [ ] **Sound (APU):** Emulate the four sound channels.
* [ ] **Additional MBCs:** Add support for MBC2, MBC3 (with RTC), and MBC5.
* [ ] **BIOS Emulation:** Load and execute the original DMG BIOS ROM.
* [ ] **Testing:** Pass Blargg's instruction and memory timing test ROMs.

---

## 🛠️ How to Build and Run

### Dependencies

You will need the **SDL2** development library to compile the project.

* **On Debian/Ubuntu:**

    ```bash
    sudo apt-get install libsdl2-dev
    ```

* **On macOS (using Homebrew):**

    ```bash
    brew install sdl2
    ```

### Compilation and Execution

1.**Clone the repository:**

```bash
git clone [https://github.com/AndyFerns/GBCee.git](https://github.com/AndyFerns/GBCee.git)
cd GBCee
```

2.**Compile the project using the Build.bat batchfile:**

```bash
./build
```

3.**Run the emulator with a Gameboy ROM:**

```bash
./gbcee /path/to/your/rom.gb
```

## Author

Andrew Fernandes :)
