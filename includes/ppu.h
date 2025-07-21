#ifndef PPU_H
#define PPU_H

#include <stdint.h>

/**
 * init_ppu - Initializes the PPU (Pixel Processing Unit).
 *
 * Resets internal registers and prepares the PPU for rendering.
 */
void init_ppu();

/**
 * ppu_step - Advances the PPU by one step (typically per CPU cycle).
 *
 * Simulates rendering phases (OAM Search, Drawing, HBlank, VBlank).
 * Should be called once per CPU cycle or as part of frame scheduling.
 */
void ppu_step();

/**
 * ppu_render_frame - Renders the full frame to the SDL window.
 *
 * Draws the current screen contents using SDL.
 * Should be called after each complete frame (VBlank).
 */
void ppu_render_frame();

#endif
