// The desktop simulator: the firmware's UI, fed by fake data, in a
// window. It draws through the same src/app code into the same RGB565
// canvas, so it's a pixel-exact preview: UI changes are developed here.
// The keyboard stands in for the rotary encoder: arrows turn, space
// clicks; r shows or hides a READY prompt, as with nothing connected (its
// Connect clears it; left alone 30s, the screensaver comes on).

#include <SDL.h>
#include <stdbool.h>

#include "data/simdata.h"
#include "ui/ui.h"

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
    SDL_Texture *texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, PANEL_WIDTH, PANEL_HEIGHT);
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
        int turn = 0;
        bool press = false;
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_DOWN:
                case SDLK_RIGHT:
                    turn++;
                    break;
                case SDLK_UP:
                case SDLK_LEFT:
                    turn--;
                    break;
                case SDLK_SPACE:
                    press = true;
                    break;
                case SDLK_r:
                    state_set_status(&state, state.status[0] ? "" : "READY\nIOS-Vlink\n(last used)");
                    state.wait = state.status[0] ? WAIT_CHOICE : WAIT_NONE;
                    break;
                }
            }
        }

        uint32_t now = SDL_GetTicks();
        if (screen_apply_input(&screen, &state, turn, press, ui_view_count, &cfg, now)) {
            if (screen.page == PAGE_GAUGES && state.wait == WAIT_CHOICE) {
                state_set_status(&state, ""); // Connect: as if it worked
                state.wait = WAIT_NONE;
            } else {
                screen_apply_click(&screen, &cfg);
            }
        }
        simdata_update(&state, now);
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
