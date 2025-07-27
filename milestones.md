# Milestones

## ROM Loading

- [x] Loads .gb files into the correct memory address (0x0000–0x7FFF).

- [x]  Begins execution at 0x0100 (Game Boy boot skip behavior).

## Basic CPU Execution

- [x] Fetch-decode-execute cycle is in place.

- [x] PC increments properly.

- [x] Registers are being updated (LD, INC, DEC, etc.).

- [x] HALT instruction is implemented and halts execution properly.

## Disassembly / Debug Logging

- [x] CPU register state is logged with each instruction — useful for debugging and correctness.

## 🔜 Next Milestones (Roadmap to a Working Emulator)

Here’s the milestone roadmap sorted into logical steps. Let’s track your progress like a checklist.

## CPU: Core Instruction Set

- [ ] Implement the full instruction set (~500 opcodes including CB-prefix ones).

- [x] All 8-bit loads: LD r1, r2, LD r, (HL), LD (HL), r

- [x] 16-bit loads: LD rr, nn, PUSH, POP, LD SP, HL

- [x] Arithmetic/logic: ADD, SUB, ADC, SBC, AND, OR, XOR, CP

- [ ] Jumps/calls/returns: JP, CALL, RET, JR, RST, conditional versions

- [x] Rotate/shift ops: RLC, RR, SLA, etc.

- [x] CB-prefixed instructions (0xCB): extended bit-level ops

- [x] Add a “not yet implemented” fallback so you can stub these gradually.

## Memory Map Implementation

- [x] Map the Game Boy’s memory layout:

- [ ] ROM Bank 0 (0x0000–0x3FFF)

- [ ] ROM Bank N (0x4000–0x7FFF) — needs banking logic for MBCs

- [ ] VRAM (0x8000–0x9FFF)

- [ ] External RAM (0xA000–0xBFFF)

- [ ] WRAM, Echo RAM, OAM, IO, etc.

- 🧠 Use a read_byte() and write_byte() abstraction for MMU.

## Input Handling

- [ ]  Map keyboard → Game Boy buttons (A, B, Start, etc.)

- [ ]  Write button state to memory-mapped IO (0xFF00)

## PPU (Graphics)

- [ ]  Implement the full PPU state machine:

- [ ]  Modes: OAM Scan, Drawing, HBlank, VBlank

- [ ]  Tile and background rendering

- [ ]  OAM sprite rendering

- [ ]  Render each scanline (160x144 screen) via SDL

- [ ]  VBlank interrupt + screen buffer flip

## APU (Sound)

- [ ] Can be done later, as many emulators are "silent" at first.

- [ ]  Emulate sound channels + timing

- [ ]  SDL2 audio support

## Timers & Interrupts

- [ ]  DIV, TIMA, TMA, TAC registers

- [ ]  Interrupt enable/disable (IME), EI, DI handling

- [ ]  Fire interrupts on VBlank, Timer, etc.

- [ ]  Vector to 0x0040, 0x0048, etc. as needed

## MBC (Memory Bank Controllers)

- [ ]  MBC0: No banking

- [ ]  MBC1: Switchable ROM/RAM banks

- [ ]  MBC2, MBC3 (RTC), MBC5 (big ROM support)

- 📦 Needed to support most real GB games.

## BIOS Emulation

You’ve likely skipped this for now (start at 0x0100).

 Load and run BIOS (0x0000–0x00FF)

 Emulate startup logo + Nintendo check

## Testing Infrastructure

- [ ] Add instruction tests (compare against known-good logs)

- [ ]  Run Blargg's test ROMs to verify CPU correctness

## Game Compatibility Milestone

- [ ] Run “Tetris” or “Dr. Mario” (No MBC)

- [ ]  Run “Pokemon Red” (MBC1)

- [ ]  Pass blargg’s instr_timing.gb

## 🗺️ Suggested Immediate Next Steps

🧩 1. Implement MMU Layer
Add mmu.c/h with mmu_read() and mmu_write()

Abstract away direct memory[addr] access

🎯 2. Expand Opcode Coverage
Implement more LD, INC, DEC, JP, JR instructions.

Stub others as printf("Unimplemented opcode: 0x%02X\n", opcode);

🖼️ 3. Build PPU Scaffold
Create ppu.c/h with function stubs:

ppu_step(), ppu_tick(), ppu_render_frame()

📌 Long-Term Optional Features
Save/load game state (savestate files)

Debugger: step, breakpoints, disassembler

GUI: Choose ROMs, inspect memory

Link cable emulation (multi-player)

Super Game Boy color support
