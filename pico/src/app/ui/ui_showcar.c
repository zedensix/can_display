// The screensaver, nothing connected: the 86's logo, then the logos of
// the parts on the car (brands.c, from logos/*.png), one at a time. Each
// fades in rising a few pixels, a shine crosses it, and it fades out
// rising on; dots along the bottom say which one it is.

#include <math.h>

#include "ui_internal.h"

#define LOGO_MS 4000 // each logo on screen
#define FADE_MS 600  // its fade in, and out
#define RISE 10      // how far it rises in each fade, pixels
#define SHINE_FROM 900
#define SHINE_MS 900
// The logo's center and the dots' top, from the bottom: the logo 140px
// down on a 320px panel, at the same share of a landscape one.
#define DOTS_FROM_BOTTOM 30

// draw_dots draws a dot per logo, the one shown a lit pill.
static void draw_dots(canvas_t *c, const theme_t *t, int count, int shown) {
    const int dot = 6, pill = 16, gap = 6;
    int x = (c->width - (count - 1) * (dot + gap) - pill) / 2;
    for (int i = 0; i < count; i++) {
        int w = i == shown ? pill : dot;
        fill_round_rect(c, x, c->height - DOTS_FROM_BOTTOM, w, dot, dot / 2,
                        i == shown ? t->accent : gfx_blend(t->background, t->label, 0.25f), t->background);
        x += w + gap;
    }
}

void draw_showcar(canvas_t *c, const theme_t *t, uint32_t now_ms) {
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    int count = 1 + brand_count, shown = (int)(now_ms / LOGO_MS % (uint32_t)count);
    uint32_t at = now_ms % LOGO_MS;

    // In: from below, fading in; out: on up, fading out.
    float in = fminf(1, (float)at / FADE_MS), out = fminf(1, (float)(LOGO_MS - at) / FADE_MS);
    float ease_in = 1 - (1 - in) * (1 - in), ease_out = 1 - (1 - out) * (1 - out);
    int y = c->height * 140 / 320 + (int)lroundf(RISE * (1 - ease_in)) - (int)lroundf(RISE * (1 - ease_out));
    float shown_fade = fminf(ease_in, ease_out);

    // The shine crosses the logo once it's in.
    int cx = c->width / 2, sweep = -1000;
    if (at >= SHINE_FROM && at < SHINE_FROM + SHINE_MS) {
        sweep = cx + y - 160 + (int)((at - SHINE_FROM) * 320 / SHINE_MS);
    }
    // The 86 in the dash's red-orange, the parts' logos in white.
    color_t color = gfx_blend(t->background, shown == 0 ? t->accent : t->alarm_text, shown_fade);
    color_t shine = gfx_blend(t->background, t->alarm_text, shown_fade);
    draw_bitmap(c, shown == 0 ? logo_bin : brands[shown - 1].bitmap, cx, y, 1, color, shine, sweep);
    if (count > 1) {
        draw_dots(c, t, count, shown);
    }
}
