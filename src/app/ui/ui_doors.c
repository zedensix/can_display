// The door screen: with a door or the boot open, the whole screen shows
// which, on the car seen from above, front up, left-hand drive (the
// driver's door on the left): the open one swung out in red. Portrait:
// the words over the car; landscape: the car on the left, the words on
// its right.

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui_internal.h"

// The car's size: the body, its wheels sticking out each side.
#define CAR_W 100
#define CAR_H 210
// How far a door swings out, degrees.
#define DOOR_SWING 42

// draw_door_ajar draws a door swung out from its hinge at x, y (the
// front end of the opening), len long, side -1 left, 1 right: a line of
// 4px squares, the panel having no line drawing.
static void draw_door_ajar(canvas_t *c, int x, int y, int len, int side, color_t color) {
    float a = DOOR_SWING * PI / 180, dx = -sinf(a) * (float)-side, dy = cosf(a);
    for (int i = 0; i <= len; i += 2) {
        gfx_fill_rect(c, x + (int)lroundf(dx * (float)i) - 2, y + (int)lroundf(dy * (float)i) - 2, 5, 5, color);
    }
}

// draw_car draws the car from above, its top left at x, y, doors open
// (DOOR_* bits) in red.
static void draw_car(canvas_t *c, const theme_t *t, int x, int y, uint8_t doors) {
    color_t body = RGB(0x15, 0x15, 0x15), edge = RGB(0x5a, 0x5a, 0x5a), glass = RGB(0x3a, 0x3a, 0x3a);
    color_t tyre = RGB(0x2a, 0x2a, 0x2a);
    for (int i = 0; i < 4; i++) {
        gfx_fill_rect(c, x + (i % 2 ? CAR_W - 14 : 0), y + (i < 2 ? 30 : 150), 14, 36, tyre);
    }
    fill_round_rect(c, x + 8, y, CAR_W - 16, CAR_H, 30, edge, t->background);
    fill_round_rect(c, x + 10, y + 2, CAR_W - 20, CAR_H - 4, 28, body, edge);
    outline_round_rect(c, x + 20, y + 60, CAR_W - 40, 92, 14, glass, body);
    // The doors' and the boot's seams, red when open.
    gfx_fill_rect(c, x + 8, y + 68, 3, 62, doors & DOOR_DRIVER ? t->danger : edge);
    gfx_fill_rect(c, x + CAR_W - 11, y + 68, 3, 62, doors & DOOR_PASSENGER ? t->danger : edge);
    gfx_fill_rect(c, x + 22, y + CAR_H - 26, CAR_W - 44, 3, doors & DOOR_BOOT ? t->danger : edge);
    if (doors & DOOR_DRIVER) {
        draw_door_ajar(c, x + 9, y + 68, 62, -1, t->danger);
    }
    if (doors & DOOR_PASSENGER) {
        draw_door_ajar(c, x + CAR_W - 10, y + 68, 62, 1, t->danger);
    }
    if (doors & DOOR_BOOT) {
        fill_round_rect(c, x + 22, y + CAR_H - 14, CAR_W - 44, 14, 3, t->danger, t->background);
    }
}

void draw_door_screen(canvas_t *c, const theme_t *t, uint8_t doors) {
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    const char *title = doors == DOOR_BOOT ? "BOOT OPEN" : "DOOR OPEN";
    // Which, all of them when more than one.
    char which[48] = "";
    static const char *const names[] = {"DRIVER", "PASSENGER", "BOOT"};
    for (int i = 0; i < 3; i++) {
        if (doors & (1 << i)) {
            size_t len = strlen(which);
            snprintf(which + len, sizeof which - len, "%s%s", len ? " + " : "", names[i]);
        }
    }
    if (is_landscape(c)) {
        // Room on the car's left for the driver's door swung out.
        draw_car(c, t, 50, (c->height - CAR_H) / 2, doors);
        int cx = 160 + (c->width - 160) / 2;
        draw_text_fitting(c, cx, c->height / 2 - 4, c->width - 160 - 8, title, t->danger);
        draw_text_centered(c, &font_bold12, cx, c->height / 2 + 22, which, t->label, 1);
        return;
    }
    draw_text_fitting(c, c->width / 2, 42, c->width - 16, title, t->danger);
    draw_text_centered(c, &font_bold12, c->width / 2, 68, which, t->label, 1);
    draw_car(c, t, (c->width - CAR_W) / 2, c->height - CAR_H - 16, doors);
}
