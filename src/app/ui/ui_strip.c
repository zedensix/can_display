// The info strip along the bottom of the gauges: the gear engaged (N
// for none), the temperature outside, and the distance (the car's
// odometer, whole km, if its ECU gives it, else the trip since boot, to
// the tenth).

#include <stdio.h>

#include "ui_internal.h"

// draw_strip_cell centers value in a cell from x, w wide, on the strip's
// baseline, all three the same size: a line of info, not gauges. No
// labels: the units say what each is.
static void draw_strip_cell(canvas_t *c, int x, int w, int baseline, const char *value, color_t color) {
    gfx_text(c, &font_bold12, x + (w - gfx_text_width(&font_bold12, value)) / 2, baseline, value, color, 1);
}

void draw_info_strip(canvas_t *c, const theme_t *t, const state_t *s, uint32_t now_ms) {
    // The gear's cell a quarter of the width, the other two the rest.
    int y = c->height - INFO_STRIP_HEIGHT, baseline = y + INFO_STRIP_HEIGHT - 3;
    int gear_w = c->width / 4, cell = (c->width - gear_w) / 2;
    gfx_fill_rect(c, GRID_GAP, y, c->width - 2 * GRID_GAP, 1, t->frame);

    int gear = state_gear(s, now_ms, FAST_STALE_MS);
    char text[16];
    snprintf(text, sizeof text, gear ? "%d" : "N", gear);
    draw_strip_cell(c, 0, gear_w, baseline, text, gear ? t->accent : t->label);

    const reading_t *outside = &s->readings[SIG_AMBIENT];
    if (reading_is_stale(outside, now_ms, SLOW_STALE_MS)) {
        snprintf(text, sizeof text, "--");
    } else {
        snprintf(text, sizeof text, "%.0f" DEGREE "C", (double)outside->value);
    }
    draw_strip_cell(c, gear_w, cell, baseline, text, t->label);

    const reading_t *odometer = &s->readings[SIG_ODOMETER];
    bool odo = !reading_is_stale(odometer, now_ms, SLOW_STALE_MS);
    snprintf(text, sizeof text, odo ? "%.0f km" : "%.1f km", (double)(odo ? odometer->value : s->trip_km));
    draw_strip_cell(c, gear_w + cell, c->width - gear_w - cell, baseline, text, t->label);
}
