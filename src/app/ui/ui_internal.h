#pragma once

// What the UI's files share. ui.c holds the theme, the gauges' ranges and
// ui_render; ui_tiles.c draws the gauges (grid tiles, a dial alone);
// ui_strip.c the info strip under them;
// ui_status.c the status screen and the boot animation; ui_settings.c the
// settings page; ui_draw.c the drawing helpers they all use.

#include "ui.h"

#define PI 3.14159265f

// Most signals update several times a second. The temperatures change
// slowly, and the ethanol % comes in the MCP2515's slower round of
// requests: they get longer.
#define FAST_STALE_MS 2000
#define SLOW_STALE_MS 10000

#define GRID_GAP 3

// A canvas wider than tall: the panel turned (settings_t.landscape).
// Each screen lays itself out for one or the other.
static inline bool is_landscape(const canvas_t *c) {
    return c->width > c->height;
}

// The info strip's height, along the bottom of the gauges page.
#define INFO_STRIP_HEIGHT 22

extern const font_t *const label_font;

// ui_draw.c
// draw_segment_bar draws count segments in a row.
void draw_segment_bar(canvas_t *c, int x, int y, int w, int h, int count, const color_t colors[]);
void draw_outline(canvas_t *c, int x, int y, int w, int h, color_t color);
// fill_round_rect and outline_round_rect draw a rectangle with corners of
// radius r (at most half its side: a pill, a circle), anti-aliased against
// bg, the color under them, like the spinner's ring.
void fill_round_rect(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg);
void outline_round_rect(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg);
typedef enum { ICON_GEAR, ICON_BACK, ICON_RETRY } icon_t;
// draw_button draws a pill button, its icon and label centered: filled
// when chosen (the knob's on it), else outlined.
void draw_button(canvas_t *c, const theme_t *t, int x, int y, int w, int h, icon_t icon, const char *label,
                 bool chosen);
void draw_text_centered(canvas_t *c, const font_t *font, int cx, int y, const char *s, color_t color, int scale);
// draw_text_fitting centers s in the largest font it fits max_width in.
void draw_text_fitting(canvas_t *c, int cx, int y, int max_width, const char *s, color_t color);
// A reading in danger blinks: its tile lit red, then not, twice a second.
bool alarm_blink_lit(uint32_t now_ms);

// ui_tiles.c: the gauges between top and bottom.
// format_tile_value is what tile shows for its reading: text, color and
// gauge fraction (-1: stale, "--").
void format_tile_value(const tile_t *tile, const state_t *s, uint32_t now_ms, const theme_t *t, char text[24],
                       color_t *color, float *fraction);
// gauge_segment_colors colors count segments of tile's range lit up to
// fraction, peak (or -1) marked, on an alarm's red when alarm.
void gauge_segment_colors(const range_t *r, float fraction, float peak, int count, bool alarm, const theme_t *t,
                          color_t out[]);
void draw_track_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, drive_mode_t mode,
                     uint32_t now_ms);
void draw_tile_dial(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                    uint32_t now_ms);

// ui_main.c
// draw_main_layout is the gauges page's every-gauge view between top
// and bottom: the oil temperature first and big, the others as a list.
void draw_main_layout(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, uint32_t now_ms);

// ui_doors.c
// draw_door_screen is the whole screen with doors (DOOR_* bits) open.
void draw_door_screen(canvas_t *c, const theme_t *t, uint8_t doors);

// ui_strip.c
// draw_info_strip draws the gear, the outside temperature and the
// distance along the canvas's bottom, INFO_STRIP_HEIGHT high.
void draw_info_strip(canvas_t *c, const theme_t *t, const state_t *s, uint32_t now_ms);

// ui_status.c
void draw_status_screen(canvas_t *c, const theme_t *t, const state_t *state, const screen_t *screen, uint32_t now_ms);
void draw_spinner(canvas_t *c, const theme_t *t, int cx, int cy, uint32_t now_ms, float done);
// draw_logo draws the logo centered on cx, cy, shrink times smaller: a
// pixel is lit if any in its shrink x shrink block is.
void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color);
// draw_bitmap draws a 1-bit bitmap as logo.c holds them (big-endian
// width and height, then rows) like draw_logo, with a band of shine
// across it on the diagonal x + y = sweep (screen pixels; far off: none).
void draw_bitmap(canvas_t *c, const unsigned char *bitmap, int cx, int cy, int shrink, color_t color, color_t shine,
                 int sweep);
extern const unsigned char logo_bin[];

// brands.c, generated from logos/*.png: the screensaver's logos after the 86's.
typedef struct {
    const char *name;
    const unsigned char *bitmap;
} brand_t;
extern const brand_t brands[];
extern const int brand_count;

// ui_showcar.c
// draw_showcar is the screensaver, nothing connected: the car on show.
void draw_showcar(canvas_t *c, const theme_t *t, uint32_t now_ms);

// ui_settings.c
void draw_settings_page(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg);
