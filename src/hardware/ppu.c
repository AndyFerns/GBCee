#include "ppu.h"
#include "mmu.h"
#include <SDL2/SDL.h>
#include <string.h>

// change according to screen sizes
#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 144

// SDL rendering suite
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static uint32_t framebuffer[SCREEN_WIDTH * SCREEN_HEIGHT];

/**
 * init_ppu - See header.
 */
void init_ppu() {
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow(
        "GameBoy Emulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH * 2,
        SCREEN_HEIGHT * 2,
        0
    );
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );
    memset(framebuffer, 0xFF, sizeof(framebuffer)); // White screen
}

/**
 * ppu_step - See header.
 */
void ppu_step() {
    // Placeholder for scanline rendering logic
    // Eventually simulate LY, LCDC, STAT, etc.
}

/**
 * ppu_render_frame - See header.
 */
void ppu_render_frame() {
    SDL_UpdateTexture(texture, NULL, framebuffer, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
