#include "ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

const theme_t theme_dark = {
    .background = RGB(0x0b, 0x0e, 0x12),
    .label = RGB(0x8a, 0x94, 0xa3),
    .good = RGB(0x34, 0xd3, 0x99),
    .warning = RGB(0xf5, 0xa6, 0x23),
    .danger = RGB(0xe5, 0x4b, 0x4b),
    .stale = RGB(0x3a, 0x40, 0x4a),
};

// Normal, warning and danger bands per gauge. -1 or one past the max
// disables a side.
#define RPM_RANGE {0, 8000, -1, 7000, -1, 7400}
#define COOLANT_RANGE {40, 130, 60, 105, 50, 115}
#define OIL_RANGE {40, 150, 60, 110, 50, 120}
#define BATTERY_RANGE {10, 16, 12, 15, 11.5f, 15.5f}
#define AFR_RANGE {10, 18, 12, 16, 11, 17}
#define ETHANOL_RANGE {0, 100, -1, 101, -1, 101}

// Most signals update several times a second. The oil temperature comes
// every 3s through the dongle (ELM_OIL_INTERVAL_MS).
#define FAST_STALE_MS 2000
#define OIL_STALE_MS 10000

const tile_t ui_tiles[] = {
    {"RPM", "%.0f", "", SIG_RPM, RPM_RANGE, FAST_STALE_MS},
    {"COOLANT", "%.0f", DEGREE "C", SIG_COOLANT, COOLANT_RANGE, FAST_STALE_MS},
    {"OIL TEMP", "%.0f", DEGREE "C", SIG_OIL, OIL_RANGE, OIL_STALE_MS},
    {"BATTERY", "%.1f", "V", SIG_BATTERY, BATTERY_RANGE, FAST_STALE_MS},
    {"AFR", "%.1f", "", SIG_AFR, AFR_RANGE, FAST_STALE_MS},
    {"ETHANOL", "%.0f", "%", SIG_ETHANOL, ETHANOL_RANGE, FAST_STALE_MS},
};
const int ui_tile_count = sizeof ui_tiles / sizeof ui_tiles[0];

extern const unsigned char logo_bin[];

#define GRID_GAP 3
#define GRID_COLUMNS 2

static const font_t *const label_font = &font_bold9;
// Tried largest first: wider readings ("-14.2V") drop a size or two to
// stay inside their tile.
static const font_t *const value_fonts[] = {&font_bold24, &font_bold18, &font_bold12};

static color_t range_color(const range_t *r, float v, const theme_t *t) {
    if (v <= r->danger_low || v >= r->danger_high) {
        return t->danger;
    }
    if (v <= r->warn_low || v >= r->warn_high) {
        return t->warning;
    }
    return t->good;
}

static float range_fraction(const range_t *r, float v) {
    if (r->max <= r->min) {
        return 0;
    }
    float f = (v - r->min) / (r->max - r->min);
    return f < 0 ? 0 : f > 1 ? 1 : f;
}

// tile_value is what tile shows for reading: its text, color and bar
// fraction, or "--" once stale.
static void tile_value(const tile_t *tile, reading_t reading, uint32_t now_ms, const theme_t *t, char text[24],
                       color_t *color, float *fraction) {
    if (reading_stale(&reading, now_ms, tile->stale_ms)) {
        snprintf(text, 24, "--");
        *color = t->stale;
        *fraction = 0;
        return;
    }
    char number[16];
    snprintf(number, sizeof number, tile->format, (double)reading.value);
    snprintf(text, 24, "%s%s", number, tile->unit);
    *color = range_color(&tile->range, reading.value, t);
    *fraction = range_fraction(&tile->range, reading.value);
}

static void draw_bar(canvas_t *c, int x, int y, int w, int h, float fraction, color_t fill, color_t track) {
    gfx_fill_rect(c, x, y, w, h, track);
    int filled = (int)((float)w * fraction);
    if (filled > 0) {
        gfx_fill_rect(c, x, y, filled, h, fill);
    }
}

static void draw_tile(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, reading_t r,
                      uint32_t now_ms) {
    gfx_fill_rect(c, x, y, w, h, t->background);
    int label_y = y + 18;
    gfx_text(c, label_font, x + 8, label_y, tile->label, t->label, 1);

    char text[24];
    color_t color;
    float fraction;
    tile_value(tile, r, now_ms, t, text, &color, &fraction);
    const font_t *font = value_fonts[2];
    for (int i = 0; i < 3; i++) {
        if (gfx_text_width(value_fonts[i], text) <= w - 16) {
            font = value_fonts[i];
            break;
        }
    }
    gfx_text(c, font, x + 8, label_y + 40, text, color, 1);
    draw_bar(c, x + 8, y + h - 12, w - 16, 6, fraction, color, t->stale);
}

// draw_tile_large draws a tile alone on the whole area, its value blown
// up as much as fits, up to 3 times: chunky, but readable from the seat,
// without a bigger font taking flash.
static void draw_tile_large(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h,
                            reading_t r, uint32_t now_ms) {
    gfx_fill_rect(c, x, y, w, h, t->background);
    gfx_text(c, label_font, x + 8, y + 24, tile->label, t->label, 1);

    char text[24];
    color_t color;
    float fraction;
    tile_value(tile, r, now_ms, t, text, &color, &fraction);
    int width = gfx_text_width(&font_bold24, text);
    int scale = 3;
    while (scale > 1 && width * scale > w - 16) {
        scale--;
    }
    // The 24pt digits rise about 26px above their baseline.
    const int cap_height = 26;
    gfx_text(c, &font_bold24, x + (w - width * scale) / 2, y + h / 2 + cap_height * scale / 2, text, color, scale);
    draw_bar(c, x + 8, y + h - 28, w - 16, 16, fraction, color, t->stale);
}

static void draw_grid(canvas_t *c, const theme_t *t, const reading_t *readings, uint32_t now_ms) {
    int rows = (ui_tile_count + GRID_COLUMNS - 1) / GRID_COLUMNS;
    int cell_w = c->width / GRID_COLUMNS, cell_h = c->height / rows;
    for (int i = 0; i < ui_tile_count; i++) {
        int col = i % GRID_COLUMNS, row = i / GRID_COLUMNS;
        draw_tile(c, t, &ui_tiles[i], col * cell_w + GRID_GAP, row * cell_h + GRID_GAP, cell_w - 2 * GRID_GAP,
                  cell_h - 2 * GRID_GAP, readings[ui_tiles[i].signal], now_ms);
    }
}

int ui_wrap(const font_t *font, const char *s, int max_width, char out[][64], int max) {
    int n = 0;
    char line[64] = "";
    while (*s && n < max) {
        while (*s == ' ') {
            s++;
        }
        size_t word_len = strcspn(s, " ");
        if (word_len == 0) {
            break;
        }
        char candidate[64];
        if (line[0]) {
            snprintf(candidate, sizeof candidate, "%s %.*s", line, (int)word_len, s);
        } else {
            snprintf(candidate, sizeof candidate, "%.*s", (int)word_len, s);
        }
        if (line[0] && gfx_text_width(font, candidate) > max_width) {
            snprintf(out[n++], 64, "%s", line);
            snprintf(line, sizeof line, "%.*s", (int)word_len, s);
        } else {
            snprintf(line, sizeof line, "%s", candidate);
        }
        s += word_len;
    }
    if (n < max && (line[0] || n == 0)) {
        snprintf(out[n++], 64, "%s", line);
    }
    return n;
}

#define STATUS_MARGIN 12
#define STATUS_LINE_HEIGHT 24
#define SPINNER_DOT 6

// The spinner's dots, clockwise from the top. Its bright head moves one
// dot every 100ms, trailed by a dimmer one, so a slow step doesn't look
// frozen.
static const int8_t spinner_dots[8][2] = {{0, -16}, {11, -11}, {16, 0}, {11, 11},
                                          {0, 16},  {-11, 11}, {-16, 0}, {-11, -11}};

static void draw_status(canvas_t *c, const theme_t *t, const char *status, uint32_t now_ms) {
    int head = (int)(now_ms / 100 % 8);
    for (int i = 0; i < 8; i++) {
        int age = (head - i + 8) % 8;
        color_t color = age == 0 ? t->warning : age == 1 ? t->label : t->stale;
        gfx_fill_rect(c, c->width / 2 + spinner_dots[i][0] - SPINNER_DOT / 2,
                      c->height - 48 + spinner_dots[i][1] - SPINNER_DOT / 2, SPINNER_DOT, SPINNER_DOT, color);
    }

    // The first paragraph is the title.
    int y = STATUS_MARGIN + STATUS_LINE_HEIGHT;
    bool first = true;
    char paragraph[192];
    snprintf(paragraph, sizeof paragraph, "%s", status);
    for (char *p = paragraph; p; first = false) {
        char *next = strchr(p, '\n');
        if (next) {
            *next++ = 0;
        }
        char lines[8][64];
        int n = ui_wrap(label_font, p, c->width - 2 * STATUS_MARGIN, lines, 8);
        for (int i = 0; i < n; i++) {
            gfx_text(c, label_font, STATUS_MARGIN, y, lines[i], first ? t->warning : t->label, 1);
            y += STATUS_LINE_HEIGHT;
        }
        p = next;
    }
}

#define SETTINGS_MARGIN 8
#define SETTINGS_TOP 48
#define SETTINGS_ROW_HEIGHT 40

static void draw_settings(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg) {
    gfx_text(c, label_font, SETTINGS_MARGIN + 4, 28, "SETTINGS", t->warning, 1);
    char brightness[8];
    snprintf(brightness, sizeof brightness, "%d%%", cfg->brightness);
    const char *rows[SETTINGS_ROWS][2] = {
        [ROW_BRIGHTNESS] = {"Brightness", brightness},
        [ROW_AUTO_CONNECT] = {"Auto-connect", cfg->auto_connect ? "ON" : "OFF"},
        [ROW_FLIP] = {"Flip screen", cfg->flip ? "ON" : "OFF"},
        [ROW_ETHANOL_SENSOR] = {"Ethanol sensor", cfg->ethanol_sensor ? "ON" : "OFF"},
        [ROW_BACK] = {"Back", ""},
    };
    for (int i = 0; i < SETTINGS_ROWS; i++) {
        int y = SETTINGS_TOP + i * SETTINGS_ROW_HEIGHT;
        color_t label = t->label, value = t->good;
        if (i == screen->cursor) {
            gfx_fill_rect(c, SETTINGS_MARGIN, y, c->width - 2 * SETTINGS_MARGIN, SETTINGS_ROW_HEIGHT - 6, t->stale);
            label = t->warning;
            if (screen->editing) {
                value = t->warning;
            }
        }
        gfx_text(c, label_font, SETTINGS_MARGIN + 6, y + 23, rows[i][0], label, 1);
        int width = gfx_text_width(label_font, rows[i][1]);
        gfx_text(c, label_font, c->width - SETTINGS_MARGIN - 6 - width, y + 23, rows[i][1], value, 1);
    }
    gfx_text(c, label_font, SETTINGS_MARGIN + 4, c->height - 14, "hold: reboot", t->stale, 1);
}

void ui_render(canvas_t *c, const state_t *state, const screen_t *screen, const settings_t *cfg, uint32_t now_ms) {
    const theme_t *t = &theme_dark;
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    if (screen->page == PAGE_SETTINGS) {
        draw_settings(c, t, screen, cfg);
    } else if (state->status[0]) {
        draw_status(c, t, state->status, now_ms);
    } else if (screen->view > 0 && screen->view <= ui_tile_count) {
        const tile_t *tile = &ui_tiles[screen->view - 1];
        draw_tile_large(c, t, tile, GRID_GAP, GRID_GAP, c->width - 2 * GRID_GAP, c->height - 2 * GRID_GAP,
                        state->readings[tile->signal], now_ms);
    } else {
        draw_grid(c, t, state->readings, now_ms);
    }
}

static void draw_logo(canvas_t *c, color_t color) {
    int w = logo_bin[0] << 8 | logo_bin[1], h = logo_bin[2] << 8 | logo_bin[3];
    int stride = (w + 7) / 8;
    int x0 = (c->width - w) / 2, y0 = (c->height - h) / 2;
    const unsigned char *rows = logo_bin + 4;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (rows[y * stride + x / 8] & (0x80 >> (x % 8))) {
                gfx_fill_rect(c, x0 + x, y0 + y, 1, 1, color);
            }
        }
    }
}

bool ui_render_boot(canvas_t *c, uint32_t elapsed_ms, uint32_t now_ms) {
    if (elapsed_ms >= BOOT_DURATION_MS) {
        return false;
    }
    const theme_t *t = &theme_dark;
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    if (elapsed_ms < BOOT_SPLASH_MS) {
        uint32_t fade = elapsed_ms;
        if (BOOT_SPLASH_MS - elapsed_ms < fade) {
            fade = BOOT_SPLASH_MS - elapsed_ms;
        }
        if (fade > BOOT_FADE_MS) {
            fade = BOOT_FADE_MS;
        }
        draw_logo(c, gfx_blend(t->background, t->danger, (float)fade / BOOT_FADE_MS));
        return true;
    }
    float sweep = sinf(3.14159265f * (float)(elapsed_ms - BOOT_SPLASH_MS) / BOOT_SWEEP_MS);
    reading_t readings[SIG_COUNT] = {0};
    for (int i = 0; i < ui_tile_count; i++) {
        const range_t *r = &ui_tiles[i].range;
        readings[ui_tiles[i].signal] = (reading_t){r->min + sweep * (r->max - r->min), true, now_ms};
    }
    draw_grid(c, t, readings, now_ms);
    return true;
}
