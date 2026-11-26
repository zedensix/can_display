// The door screen: with a door or the boot open, the whole screen shows
// which, on the car seen from above, front up, left-hand drive (the
// driver's door on the left): the open one swung out in red, no words.
// A Dismiss button beside the car: a
// click puts the screen away (screen_apply_input).

#include <math.h>

#include "ui_internal.h"

// The car's size: the body, its wheels sticking out each side.
#define CAR_W 100
#define CAR_H 210
// How far a door swings out, degrees.
#define DOOR_SWING 42
#define BUTTON_H 30

// draw_door_ajar draws a door swung out from its hinge at x, y (the
// front end of the opening), len long, side -1 left, 1 right: a line of
// 4px squares, the panel having no line drawing.
static void draw_door_ajar(canvas_t *c, int x, int y, int len, int side, color_t color) {
    float a = DOOR_SWING * PI / 180, dx = -sinf(a) * (float)-side, dy = cosf(a);
    for (int i = 0; i <= len; i += 2) {
        gfx_fill_rect(c, x + (int)lroundf(dx * (float)i) - 2, y + (int)lroundf(dy * (float)i) - 2, 5, 5, color);
    }
}

// The GT86 from above, as half widths down its length (y, half width),
// interpolated between: a short nose, the front wings bulging over the
// wheels, the waist pulled in at the doors, the rear haunches wider still,
// a squared tail with its ducktail lip.
typedef struct {
    int y, half;
} profile_t;
static const profile_t body_profile[] = {
    {0, 22},   {3, 30},   {8, 36},   {16, 40},  {30, 43},  {46, 44},  {60, 42},  {74, 38},
    {100, 37}, {126, 38}, {142, 42}, {160, 45}, {178, 44}, {194, 41}, {204, 37}, {CAR_H, 32},
};
// The greenhouse: windscreen, roof, rear window.
static const profile_t glass_profile[] = {
    {78, 22}, {92, 30}, {150, 30}, {164, 26}, {174, 20},
};
// The roof panel inside it, between the windscreen and the rear window.
static const profile_t roof_profile[] = {
    {98, 25},
    {146, 25},
};
#define PROFILE(p) (p), (int)(sizeof(p) / sizeof(p)[0])
// The doors' front and rear ends, and the boot lid's front edge.
#define DOOR_FRONT 76
#define DOOR_REAR 130
#define BOOT_FRONT 180

// half_width is the profile's half width at y, 0 outside it.
static int half_width(const profile_t *p, int n, int y) {
    if (y < p[0].y || y > p[n - 1].y) {
        return 0;
    }
    int i = 1;
    while (i < n - 1 && p[i].y < y) {
        i++;
    }
    const profile_t *a = &p[i - 1], *b = &p[i];
    return b->y == a->y ? b->half : a->half + (b->half - a->half) * (y - a->y) / (b->y - a->y);
}

// fill_profile fills p's shape centered on cx, top at y0, ringed with a
// 2px edge (none: edge == fill).
static void fill_profile(canvas_t *c, int cx, int y0, const profile_t *p, int n, color_t fill, color_t edge) {
    for (int y = p[0].y; y <= p[n - 1].y; y++) {
        int h = half_width(p, n, y);
        gfx_fill_rect(c, cx - h, y0 + y, 2 * h, 1, edge);
        // Inside the edge: as narrow as the rows around it, so a slope
        // keeps its edge as thick as a side.
        if (y - p[0].y < 2 || p[n - 1].y - y < 2) {
            continue;
        }
        int in = h;
        for (int d = -2; d <= 2; d++) {
            int other = half_width(p, n, y + d);
            in = other < in ? other : in;
        }
        gfx_fill_rect(c, cx - in + 2, y0 + y, 2 * in - 4, 1, fill);
    }
}

// draw_car draws the car from above, its top left at x, y, doors open
// (DOOR_* bits) in red.
static void draw_car(canvas_t *c, const theme_t *t, int x, int y, uint8_t doors) {
    color_t body = RGB(0x15, 0x15, 0x15), edge = RGB(0x5a, 0x5a, 0x5a), glass = RGB(0x30, 0x34, 0x3a);
    color_t tyre = RGB(0x2a, 0x2a, 0x2a), light = RGB(0x9a, 0x9a, 0x9a);
    int cx = x + CAR_W / 2;
    // The tyres, just showing past the wings.
    for (int i = 0; i < 4; i++) {
        int ty = y + (i < 2 ? 24 : 146);
        gfx_fill_rect(c, i % 2 ? cx + 40 : cx - 48, ty, 8, 32, tyre);
    }
    fill_profile(c, cx, y, PROFILE(body_profile), body, edge);
    fill_profile(c, cx, y, PROFILE(glass_profile), glass, glass);
    fill_profile(c, cx, y, PROFILE(roof_profile), body, body);
    // The wings' peaks along the bonnet.
    for (int wy = 10; wy < 74; wy++) {
        int h = half_width(PROFILE(body_profile), wy) - 9;
        gfx_fill_rect(c, cx - h, y + wy, 1, 1, edge);
        gfx_fill_rect(c, cx + h, y + wy, 1, 1, edge);
    }
    // The headlights, swept back into the wings.
    for (int i = 0; i < 8; i++) {
        int w = 14 - i;
        gfx_fill_rect(c, cx - half_width(PROFILE(body_profile), 6 + i) + 3, y + 6 + i, w, 1, light);
        gfx_fill_rect(c, cx + half_width(PROFILE(body_profile), 6 + i) - 3 - w, y + 6 + i, w, 1, light);
    }
    // The doors' sides, red when open, and their seams.
    for (int side = -1; side <= 1; side += 2) {
        bool open = doors & (side < 0 ? DOOR_DRIVER : DOOR_PASSENGER);
        for (int dy = DOOR_FRONT; dy <= DOOR_REAR; dy++) {
            int h = half_width(PROFILE(body_profile), dy);
            gfx_fill_rect(c, side < 0 ? cx - h : cx + h - 3, y + dy, 3, 1, open ? t->danger : edge);
        }
        for (int k = 0; k < 2; k++) {
            int dy = k ? DOOR_REAR : DOOR_FRONT, h = half_width(PROFILE(body_profile), dy);
            gfx_fill_rect(c, side < 0 ? cx - h : cx + h - 8, y + dy, 8, 1, edge);
        }
        if (open) {
            int h = half_width(PROFILE(body_profile), DOOR_FRONT);
            draw_door_ajar(c, side < 0 ? cx - h + 1 : cx + h - 2, y + DOOR_FRONT, DOOR_REAR - DOOR_FRONT, side,
                           t->danger);
        }
    }
    // The boot lid: its seam, filled red when open.
    int bh = half_width(PROFILE(body_profile), BOOT_FRONT) - 10;
    if (doors & DOOR_BOOT) {
        for (int by = BOOT_FRONT; by < CAR_H - 3; by++) {
            int h = half_width(PROFILE(body_profile), by) - 10;
            gfx_fill_rect(c, cx - h, y + by, 2 * h, 1, t->danger);
        }
    } else {
        gfx_fill_rect(c, cx - bh, y + BOOT_FRONT, 2 * bh, 1, edge);
    }
}

void draw_door_screen(canvas_t *c, const theme_t *t, uint8_t doors) {
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    // The car a little left of center, room on its left for the driver's
    // door swung out; the button on its right.
    draw_car(c, t, 70, (c->height - CAR_H) / 2, doors);
    draw_button(c, t, 196, (c->height - BUTTON_H) / 2, c->width - 196 - 12, BUTTON_H, ICON_BACK, "Dismiss", true);
}
