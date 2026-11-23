#include <math.h>
#include <string.h>

#include "ui_internal.h"

const font_t *const label_font = &font_bold9;

void draw_segment_bar(canvas_t *c, int x, int y, int w, int h, int count, const color_t colors[]) {
    const int gap = 2;
    int seg = (w + gap) / count;
    x += (w - (seg * count - gap)) / 2;
    for (int i = 0; i < count; i++) {
        gfx_fill_rect(c, x + i * seg, y, seg - gap, h, colors[i]);
    }
}

void draw_outline(canvas_t *c, int x, int y, int w, int h, color_t color) {
    gfx_fill_rect(c, x, y, w, 1, color);
    gfx_fill_rect(c, x, y + h - 1, w, 1, color);
    gfx_fill_rect(c, x, y, 1, h, color);
    gfx_fill_rect(c, x + w - 1, y, 1, h, color);
}

void draw_text_centered(canvas_t *c, const font_t *font, int cx, int y, const char *s, color_t color, int scale) {
    gfx_text(c, font, cx - gfx_text_width(font, s) * scale / 2, y, s, color, scale);
}

#define ALARM_BLINK_MS 250

bool alarm_blink_lit(uint32_t now_ms) {
    return now_ms / ALARM_BLINK_MS % 2 == 0;
}

void draw_text_fitting(canvas_t *c, int cx, int y, int max_width, const char *s, color_t color) {
    static const font_t *const fonts[] = {&font_bold24, &font_bold18, &font_bold12, &font_bold9};
    int i = 0;
    while (i < 3 && gfx_text_width(fonts[i], s) > max_width) {
        i++;
    }
    draw_text_centered(c, fonts[i], cx, y, s, color, 1);
}

// round_corners draws every pixel of a radius r corner, with its
// distance from the corner's center (pixel centers), mirrored into the
// four corners of the w x h rectangle at x, y.
static void round_corners(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg, bool outline) {
    for (int j = 0; j < r; j++) {
        for (int i = 0; i < r; i++) {
            float dx = (float)r - (float)i - 0.5f, dy = (float)r - (float)j - 0.5f;
            float d = sqrtf(dx * dx + dy * dy);
            // Anti-aliased: the share of the pixel inside the corner's arc.
            float cover = outline ? 1 - fabsf(d - ((float)r - 0.5f)) : (float)r - d + 0.5f;
            if (cover <= 0) {
                continue;
            }
            color_t px = gfx_blend(bg, color, cover > 1 ? 1 : cover);
            gfx_fill_rect(c, x + i, y + j, 1, 1, px);
            gfx_fill_rect(c, x + w - 1 - i, y + j, 1, 1, px);
            gfx_fill_rect(c, x + i, y + h - 1 - j, 1, 1, px);
            gfx_fill_rect(c, x + w - 1 - i, y + h - 1 - j, 1, 1, px);
        }
    }
}

static int corner_radius(int w, int h, int r) {
    r = r < w / 2 ? r : w / 2;
    return r < h / 2 ? r : h / 2;
}

void fill_round_rect(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg) {
    r = corner_radius(w, h, r);
    gfx_fill_rect(c, x + r, y, w - 2 * r, h, color);
    gfx_fill_rect(c, x, y + r, r, h - 2 * r, color);
    gfx_fill_rect(c, x + w - r, y + r, r, h - 2 * r, color);
    round_corners(c, x, y, w, h, r, color, bg, false);
}

void outline_round_rect(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg) {
    r = corner_radius(w, h, r);
    gfx_fill_rect(c, x + r, y, w - 2 * r, 1, color);
    gfx_fill_rect(c, x + r, y + h - 1, w - 2 * r, 1, color);
    gfx_fill_rect(c, x, y + r, 1, h - 2 * r, color);
    gfx_fill_rect(c, x + w - 1, y + r, 1, h - 2 * r, color);
    round_corners(c, x, y, w, h, r, color, bg, true);
}

// The buttons' icons, a row of pixels per string, X lit.
static const char *const icon_rows[][13] = {
    [ICON_GEAR] = {"....XXX....", ".XX.XXX.XX.", ".XXXXXXXXX.", "..XXX.XXX..", "XXXX...XXXX", "XXX.....XXX",
                   "XXXX...XXXX", "..XXX.XXX..", ".XXXXXXXXX.", ".XX.XXX.XX.", "....XXX...."},
    [ICON_RETRY] = {"....XXXX..X.", "..XXX..XXXX.", ".XX......XX.", ".X......XXX.", "XX..........", "X...........",
                    "X...........", "XX.........X", ".X........XX", ".XX......XX.", "..XXX..XXX..", "....XXXX...."},
    [ICON_BACK] = {"...X.......", "..XX.......", ".XXXXXXXXXX", "XXXXXXXXXXX", ".XXXXXXXXXX", "..XX.......",
                   "...X......."},
};

static int icon_height(icon_t icon) {
    int h = 0;
    while (h < 13 && icon_rows[icon][h]) {
        h++;
    }
    return h;
}

static void draw_icon(canvas_t *c, icon_t icon, int x, int y, color_t color) {
    for (int row = 0; row < icon_height(icon); row++) {
        for (int col = 0; icon_rows[icon][row][col]; col++) {
            if (icon_rows[icon][row][col] == 'X') {
                gfx_fill_rect(c, x + col, y + row, 1, 1, color);
            }
        }
    }
}

void draw_button(canvas_t *c, const theme_t *t, int x, int y, int w, int h, icon_t icon, const char *label,
                 bool chosen) {
    if (chosen) {
        fill_round_rect(c, x, y, w, h, h / 2, t->accent, t->background);
    } else {
        outline_round_rect(c, x, y, w, h, h / 2, gfx_blend(t->background, t->accent, 0.5f), t->background);
    }
    color_t ink = chosen ? t->background : t->accent;
    // The icon and the label centered together, 6px apart.
    int icon_w = (int)strlen(icon_rows[icon][0]), ih = icon_height(icon);
    int total = icon_w + 6 + gfx_text_width(label_font, label), left = x + (w - total) / 2;
    draw_icon(c, icon, left, y + (h - ih) / 2, ink);
    gfx_text(c, label_font, left + icon_w + 6, y + h / 2 + 5, label, ink, 1);
}
