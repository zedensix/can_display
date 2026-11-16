#pragma once

// What render_ui.c, which writes the pictures, and render_ui_shots.c,
// which draws each screen, share.

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

// shots are the screens drawn, in the sheet's order.
extern const shot_t shots[];
extern const int shot_count;
