#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Test", 100, 100, 640, 480, 0);
    if (!win) {
        printf("SDL Window Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Delay(5000);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}