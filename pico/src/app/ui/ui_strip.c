// The info strip along the bottom of the gauges, the Android app's: the
// gear engaged (N for none) in a box outlined in the accent, the
// temperature outside, and on the right
// the distance: the car's odometer, whole km in groups of three, if its
// ECU gives it, else the trip since boot, to the tenth. No labels: the
// units say what each is.

#include <stdio.h>

#include "ui_internal.h"

void gear_text(const state_t *s, uint32_t now_ms, char out[4]) {
    int gear = state_gear(s, now_ms, FAST_STALE_MS);
    snprintf(out, 4, gear > 0 && gear < 10 ? "%d" : "N", gear);
}

// km in whole kilometres, a space between each group of three digits.
static void odometer_text(float km, char out[24]) {
    long whole = (long)km;
    if (whole < 1000) {
        snprintf(out, 24, "%ld km", whole);
    } else if (whole < 1000000) {
        snprintf(out, 24, "%ld %03ld km", whole / 1000, whole % 1000);
    } else {
        snprintf(out, 24, "%ld %03ld %03ld km", whole / 1000000, whole / 1000 % 1000, whole % 1000);
    }
}

void draw_info_strip(canvas_t *c, const theme_t *t, const state_t *s, uint32_t now_ms) {
    int y = c->height - INFO_STRIP_HEIGHT, baseline = c->height - 6;
    gfx_fill_rect(c, 0, y, c->width, 1, t->frame);
    char text[24];
    gear_text(s, now_ms, text);
    stroke_round_rect(c, 2, y + 3, 24, INFO_STRIP_HEIGHT - 5, 5, 2, t->accent, t->background);
    draw_text_centered(c, &font_text, 14, baseline, text, t->accent, 0);
    int x = 36;
    snprintf(text, sizeof text, "--" DEGREE "C");
    const reading_t *outside = &s->readings[SIG_AMBIENT];
    if (!reading_is_stale(outside, now_ms, SLOW_STALE_MS)) {
        snprintf(text, sizeof text, "%.0f" DEGREE "C", (double)outside->value);
    }
    gfx_text(c, &font_text, x, baseline, text, t->label, 0);

    const reading_t *odometer = &s->readings[SIG_ODOMETER];
    if (!reading_is_stale(odometer, now_ms, SLOW_STALE_MS)) {
        odometer_text(odometer->value, text);
    } else {
        snprintf(text, sizeof text, "%.1f km", (double)s->trip_km);
    }
    draw_text_right(c, &font_text, c->width - 4, baseline, text, t->label, 0);
}
