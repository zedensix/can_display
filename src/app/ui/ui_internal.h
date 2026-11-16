#pragma once

// What the UI's files share. ui.c holds the theme, the gauges' ranges and
// ui_render; ui_tiles.c draws the gauges (grid tiles, a dial alone);
// ui_status.c the status screen and the boot animation; ui_settings.c the
// settings page; ui_draw.c the drawing helpers they all use.

#include "ui.h"

#define PI 3.14159265f

// Most signals update several times a second. The oil temperature and
// the ethanol % come every 3s through the dongle (ELM_ENGINE_INTERVAL_MS).
#define FAST_STALE_MS 2000
#define SLOW_STALE_MS 10000

#define GRID_GAP 3
#define GRID_COLUMNS 2

extern const font_t *const label_font;

// ui_draw.c
// draw_segment_bar draws count segments in a row.
void draw_segment_bar(canvas_t *c, int x, int y, int w, int h, int count, const color_t colors[]);
void draw_outline(canvas_t *c, int x, int y, int w, int h, color_t color);
void draw_text_centered(canvas_t *c, const font_t *font, int cx, int y, const char *s, color_t color, int scale);
// draw_text_fitting centers s in the largest font it fits max_width in.
void draw_text_fitting(canvas_t *c, int cx, int y, int max_width, const char *s, color_t color);
// A reading in danger blinks: its tile lit red, then not, twice a second.
bool alarm_blink_lit(uint32_t now_ms);

// ui_tiles.c
void draw_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, uint32_t now_ms);
void draw_tile_dial(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                    uint32_t now_ms);

// ui_status.c
void draw_status_screen(canvas_t *c, const theme_t *t, const state_t *state, const screen_t *screen, uint32_t now_ms);
void draw_spinner(canvas_t *c, const theme_t *t, int cx, int cy, uint32_t now_ms, float done);
// draw_logo draws the logo centered on cx, cy, shrink times smaller: a
// pixel is lit if any in its shrink x shrink block is.
void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color);
// draw_logo_shine is draw_logo with a band of shine across it, on the
// diagonal x + y = sweep (screen pixels).
void draw_logo_shine(canvas_t *c, int cx, int cy, int shrink, color_t color, color_t shine, int sweep);

// ui_showcar.c
// draw_showcar is the screensaver, nothing connected: the car on show.
void draw_showcar(canvas_t *c, const theme_t *t, uint32_t now_ms);

// ui_settings.c
void draw_settings_page(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg);
