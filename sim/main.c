// The desktop simulator: the firmware's UI, fed by fake data, in a
// window. It draws through the same src/app code into the same RGB565
// canvas, so it's a pixel-exact preview: UI changes are developed here.
// The keyboard stands in for the rotary encoder: arrows turn, space
// clicks.

#include <SDL.h>
#include <stdbool.h>

#include "simdata.h"
#include "ui.h"

#define PANEL_WIDTH 240
#define PANEL_HEIGHT 320
// SCALE blows the panel up so it's visible on a desktop display.
#define SCALE 3

static color_t pixels[PANEL_WIDTH * PANEL_HEIGHT];

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("GT86 Dash Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          PANEL_WIDTH * SCALE, PANEL_HEIGHT * SCALE, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                             PANEL_WIDTH, PANEL_HEIGHT);
    if (!window || !renderer || !texture) {
        SDL_Log("SDL: %s", SDL_GetError());
        return 1;
    }

    canvas_t canvas = {pixels, PANEL_WIDTH, PANEL_HEIGHT, 0, PANEL_HEIGHT};
    state_t state = {0};
    screen_t screen = {0};
    settings_t cfg = SETTINGS_DEFAULT;
    uint32_t start = SDL_GetTicks();

    for (bool running = true; running;) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_DOWN:
                case SDLK_RIGHT:
                    screen_turn(&screen, 1, ui_tile_count, &cfg);
                    break;
                case SDLK_UP:
                case SDLK_LEFT:
                    screen_turn(&screen, -1, ui_tile_count, &cfg);
                    break;
                case SDLK_SPACE:
                    screen_press(&screen, &cfg);
                    break;
                }
            }
        }

        uint32_t now = SDL_GetTicks();
        simdata_tick(&state, now);
        if (!ui_render_boot(&canvas, now - start, now)) {
            ui_render(&canvas, &state, &screen, &cfg, now);
        }
        SDL_UpdateTexture(texture, NULL, pixels, PANEL_WIDTH * sizeof pixels[0]);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    SDL_Quit();
    return 0;
}
