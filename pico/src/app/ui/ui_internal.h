#pragma once

// What the UI's files share. ui.c holds the theme, the gauges' ranges and
// ui_render; ui_tiles.c what a gauge shows (its text, color, segments);
// ui_dial.c a gauge as a round dial, and a gauge alone; ui_main.c the
// every-gauge view and the track layout; ui_strip.c the info strip under
// them; ui_status.c the status screen and the boot animation;
// ui_settings.c the settings page; ui_draw.c the drawing helpers they all
// use. The look is the Android app's (android/.../Pages.kt), laid out for
// the panel in landscape, 320 by 240.

#include "ui.h"

#define PI 3.14159265f

// The info strip along the bottom of the gauges page, its line at the top.
#define INFO_STRIP_HEIGHT 26

// The shift light across the top of the gauges, when on.
#define SHIFT_BAR_HEIGHT 14

// ui_draw.c
// draw_segment_bar draws count segments in a row, gap apart.
void draw_segment_bar(canvas_t *c, int x, int y, int w, int h, int count, int gap, const color_t colors[]);
void draw_outline(canvas_t *c, int x, int y, int w, int h, color_t color);
// fill_round_rect and outline_round_rect draw a rectangle with corners of
// radius r (at most half its side: a pill, a circle), anti-aliased against
// bg, the color under them, like the spinner's ring; stroke_round_rect
// outlines it width thick.
void fill_round_rect(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg);
void outline_round_rect(canvas_t *c, int x, int y, int w, int h, int r, color_t color, color_t bg);
void stroke_round_rect(canvas_t *c, int x, int y, int w, int h, int r, int width, color_t color, color_t bg);
typedef enum { ICON_GEAR, ICON_BACK, ICON_RETRY } icon_t;
// draw_button draws a pill button, its icon and label centered: filled
// when chosen (the knob's on it), else outlined.
void draw_button(canvas_t *c, const theme_t *t, int x, int y, int w, int h, icon_t icon, const char *label,
                 bool chosen);
void draw_text_centered(canvas_t *c, const font_t *font, int cx, int y, const char *s, color_t color, int spacing);
void draw_text_right(canvas_t *c, const font_t *font, int right, int y, const char *s, color_t color, int spacing);
// draw_text_fitting centers s in the largest font it fits max_width in.
void draw_text_fitting(canvas_t *c, int cx, int y, int max_width, const char *s, color_t color, int spacing);
// fill_arc fills the ring between r_in and r_out around cx, cy from a0 to
// a1 degrees (clockwise, 0 pointing right), anti-aliased.
void fill_arc(canvas_t *c, float cx, float cy, float r_in, float r_out, float a0, float a1, color_t color);
// A reading in danger blinks: its row lit red, then not, twice a second.
bool alarm_blink_lit(uint32_t now_ms);

// ui_tiles.c
// shown_t is what a gauge shows: its number (the unit apart), its color
// (dim "--" once stale; white off the throttle for the AFR, no mixture to
// judge), where it sits on its scale (-1: stale), and whether it's in
// danger (its row blinks).
typedef struct {
    char number[16];
    color_t color;
    float fraction;
    bool danger;
} shown_t;
shown_t tile_shown(const tile_t *tile, const state_t *s, uint32_t now_ms, const theme_t *t);
// tile_number formats v as tile does, without its unit.
void tile_number(const tile_t *tile, float v, char out[16]);
color_t band_color(const range_t *r, float v, const theme_t *t);
float scale_fraction(const range_t *r, float v);
// gauge_segment_colors colors count segments of range lit up to fraction
// in their band's color, the rest faint, so the red zone shows at rest.
void gauge_segment_colors(const range_t *r, float fraction, int count, const theme_t *t, color_t out[]);
const tile_t *tile_for(signal_t signal);

// ui_dial.c
// dial_style_t is a dial's size: its ring, its segments, its text.
typedef struct {
    int ring, segments;
    const font_t *label, *value, *unit, *extra;
} dial_style_t;
// draw_dial draws tile as a round dial in the size x size square at x, y:
// segments over 270 degrees, a glow under the lit ones, the session's
// highest marked (and its lowest, low), its label, value and unit
// inside, extra under them.
void draw_dial(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int size, const dial_style_t *style,
               const state_t *s, const char *extra, bool low, uint32_t now_ms);
// draw_scale_ends writes the scale's ends under a dial's opening.
void draw_scale_ends(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int size);
// draw_alone_page is one gauge alone between top and bottom: a big dial,
// then the session's lowest and highest and what its colors mean beside
// it.
void draw_alone_page(canvas_t *c, const theme_t *t, const tile_t *tile, int top, int bottom, const state_t *s,
                     uint32_t now_ms);

// ui_main.c
// draw_main_layout is the gauges page's every-gauge view between top
// and bottom: the oil's dial, the others as rows.
void draw_main_layout(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, uint32_t now_ms);
// draw_track_grid is the gauges in a sport mode, on track: the oil's and
// the AFR's dials across the width, the coolant and the intake as rows.
void draw_track_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, int bottom, uint32_t now_ms);

// ui_doors.c
// draw_door_screen is the whole screen with doors (DOOR_* bits) open.
void draw_door_screen(canvas_t *c, const theme_t *t, uint8_t doors);

// ui_strip.c
// draw_info_strip draws the gear, the outside temperature and the
// distance along the canvas's bottom, INFO_STRIP_HEIGHT high.
void draw_info_strip(canvas_t *c, const theme_t *t, const state_t *s, uint32_t now_ms);
// gear_text is the gear engaged, N for none.
void gear_text(const state_t *s, uint32_t now_ms, char out[4]);

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
