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
