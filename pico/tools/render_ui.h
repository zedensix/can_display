#pragma once

// What render_ui.c, which writes the pictures, and render_ui_shots.c and
// render_ui_screens.c, which draw each screen, share.

#include "ui/ui.h"

// The panel's size, as in src/firmware/board.h.
#define W 240
#define H 320

// A screen drawn: its name, how, and whether on the panel turned a
// quarter turn (H wide, W high).
typedef struct {
    const char *name;
    void (*draw)(canvas_t *c);
    bool landscape;
} shot_t;

// The screens drawn, the gauges then the rest, in the sheet's order.
extern const shot_t gauge_shots[], screen_shots[];
extern const int gauge_shot_count, screen_shot_count;

// render_ui_shots.c's fixed state: the time every shot is drawn at, a
// session while driving, the settings, and ui_render with them.
extern const uint32_t now;
extern settings_t cfg;
state_t state_while_driving(void);
void render_screen(canvas_t *c, const state_t *s, screen_t screen);
