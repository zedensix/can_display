// The desktop simulator: the firmware's UI, fed by fake data, in a
// window. It draws through the same src/app code into the same RGB565
// canvas, so it's a pixel-exact preview: UI changes are developed here.
// The keyboard stands in for the rotary encoder: arrows turn, space
// clicks; r shows or hides the MCP2515 waiting for the car to wake the bus
// (left alone 30s, the screensaver comes on); m steps
// through the drive modes (comfort, drift, sport, track, race). The
// panel is in landscape, as in the car. The fake data is a tour of the dash's scenarios (simdata.c).

#include <SDL.h>
#include <stdbool.h>

#include "data/simdata.h"
#include "obd/mcp_check.h"
#include "ui/ui.h"

#define PANEL_WIDTH 240
#define PANEL_HEIGHT 320
// The panel on its side, as in the car.
#define CANVAS_WIDTH PANEL_HEIGHT
#define CANVAS_HEIGHT PANEL_WIDTH
// SCALE blows the panel up so it's visible on a desktop display.
#define SCALE 3

static color_t pixels[PANEL_WIDTH * PANEL_HEIGHT];

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("GT86 Dash Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          CANVAS_WIDTH * SCALE, CANVAS_HEIGHT * SCALE, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, CANVAS_WIDTH, CANVAS_HEIGHT);
    if (!window || !renderer || !texture) {
        SDL_Log("SDL: %s", SDL_GetError());
        return 1;
    }

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
                case SDLK_m:
                    state_set_drive_mode(&state,
                                         (drive_mode_t)((state_drive_mode(&state, SDL_GetTicks()) + 1) % DRIVE_MODES),
                                         SDL_GetTicks());
                    break;
                case SDLK_r:
                    state_set_status(&state, state.status[0] ? "" : mcp_bus_status(BUS_SILENT));
                    break;
                }
            }
        }

        uint32_t now = SDL_GetTicks();
        if (screen_apply_input(&screen, &state, turn, press, ui_view_count, &cfg, now)) {
            screen_apply_click(&screen, &cfg);
        }
        simdata_update(&state, now);
        if (state.drive_mode_read) {
            state.drive_mode_ms = now; // as if read again: it stays until m changes it
        }
        canvas_t canvas = {pixels, CANVAS_WIDTH, CANVAS_HEIGHT, 0, CANVAS_HEIGHT};
        if (!ui_render_boot(&canvas, now - start, now)) {
            ui_render(&canvas, &state, &screen, &cfg, now);
        }
        SDL_UpdateTexture(texture, NULL, pixels, canvas.width * (int)sizeof pixels[0]);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    SDL_Quit();
    return 0;
}
