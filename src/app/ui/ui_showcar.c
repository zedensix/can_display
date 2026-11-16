// The screensaver, nothing connected: the car on show, like a show car's
// stand at night. Scanners sweep the LED bars along the top and the
// bottom, the logo turns in its ring with a shine crossing it now and
// then, a spotlight lights the floor, GT86 breathes under it, and the
// line under that changes every few seconds.

#include <math.h>

#include "ui_internal.h"

#define SCAN_LEDS 20
#define SCAN_MS 1800 // one sweep across and back
#define SCAN_TRAIL 4.0f
#define SHINE_MS 3500 // a shine across the logo, then a pause
#define BREATH_MS 2600
#define LINE_MS 4000
#define LOGO_Y 108
#define NAME_Y 264

// draw_scanner draws a bar of LEDs at y, a lit one sweeping across and
// back with a fading trail; mirrored runs it the other way.
static void draw_scanner(canvas_t *c, const theme_t *t, int y, uint32_t now_ms, bool mirrored) {
    const int gap = 2, margin = GRID_GAP;
    int led_w = (c->width - 2 * margin - gap * (SCAN_LEDS - 1)) / SCAN_LEDS;
    int x0 = (c->width - (led_w * SCAN_LEDS + gap * (SCAN_LEDS - 1))) / 2;
    float phase = (float)(now_ms % SCAN_MS) / SCAN_MS;
    float head = (phase < 0.5f ? phase * 2 : 2 - phase * 2) * (SCAN_LEDS - 1);
    if (mirrored) {
        head = SCAN_LEDS - 1 - head;
    }
    for (int i = 0; i < SCAN_LEDS; i++) {
        float d = fabsf((float)i - head);
        float lit = d < 1 ? 1 : fmaxf(0, 1 - (d - 1) / SCAN_TRAIL);
        color_t color = gfx_blend(gfx_blend(t->background, t->accent, 0.12f), lit > 0.8f ? t->danger : t->accent, lit);
        gfx_fill_rect(c, x0 + i * (led_w + gap), y, led_w, 8, color);
    }
}

// draw_spotlight lights the floor under the car: rows brighter toward
// its middle.
static void draw_spotlight(canvas_t *c, const theme_t *t) {
    const int top = LOGO_Y + 60, middle = LOGO_Y + 120, bottom = c->height - 16;
    for (int y = top; y < bottom; y++) {
        float f = 1 - fabsf((float)(y - middle)) / (float)(y < middle ? middle - top : bottom - middle);
        gfx_fill_rect(c, 0, y, c->width, 1, gfx_blend(t->background, t->frame, 0.55f * f * f));
    }
}

void draw_showcar(canvas_t *c, const theme_t *t, uint32_t now_ms) {
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    draw_spotlight(c, t);
    draw_scanner(c, t, 6, now_ms, false);
    draw_scanner(c, t, c->height - 14, now_ms, true);

    // The shine crosses the logo's diagonal in the first 60% of its cycle.
    int cx = c->width / 2;
    float shine = (float)(now_ms % SHINE_MS) / SHINE_MS / 0.6f;
    int from = cx + LOGO_Y - 90, to = cx + LOGO_Y + 90;
    int sweep = shine < 1 ? from + (int)(shine * (float)(to - from)) : -1000;
    draw_spinner(c, t, cx, LOGO_Y, now_ms, 0);
    draw_logo_shine(c, cx, LOGO_Y, 2, t->accent, t->alarm_text, sweep);

    float breath = 0.5f + 0.5f * sinf(2 * PI * (float)(now_ms % BREATH_MS) / BREATH_MS);
    draw_text_centered(c, &font_bold24, cx, NAME_Y, "GT86", gfx_blend(t->frame, t->accent, 0.45f + 0.55f * breath), 2);

    // Each line fades in and out over its LINE_MS.
    static const char *const lines[] = {"FA20 BOXER 2.0", "REAR WHEEL DRIVE", "PRESS TO WAKE"};
    uint32_t n = now_ms / LINE_MS % (sizeof lines / sizeof lines[0]);
    float fade = sinf(PI * (float)(now_ms % LINE_MS) / LINE_MS);
    draw_text_centered(c, label_font, cx, NAME_Y + 26, lines[n], gfx_blend(t->background, t->label, fade), 1);
}
