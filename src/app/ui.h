#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"
#include "screen.h"
#include "settings.h"
#include "signals.h"

// theme_t is the palette: dark for readability in a sunlit car, warning
// colors reserved for values actually out of range.
// theme_t: good, warning, danger and cold color a reading by its band; a
// reading in danger also blinks its tile to alarm with alarm_text on it.
// accent is everything around the readings (logo, titles, settings).
// frame outlines tiles, stale is what isn't lit (old readings, unlit
// segments).
typedef struct {
    color_t background, frame, accent, label, good, warning, danger, alarm, alarm_text, cold, stale;
} theme_t;

extern const theme_t theme_dark;

// range_t holds a gauge's thresholds: the bar's endpoints, and the bands
// outside which the value turns to the warning, then danger color.
// range_t is a gauge's scale and bands: warning and danger on each side
// (-1 or one past the max disables a side), and below cold (-1: none) an
// engine not warm yet, not to be revved hard.
typedef struct {
    float min, max;
    float warn_low, warn_high;
    float danger_low, danger_high;
    float cold;
} range_t;

// tile_t is one gauge: a label, the value formatted with format then
// unit, and a bar colored by range. Once its reading is older than
// stale_ms it shows "--": longer than the time between two updates of
// that signal, or it blinks.
typedef struct {
    const char *label;
    const char *format;
    const char *unit;
    signal_t signal;
    range_t range;
    uint32_t stale_ms;
} tile_t;

// ui_tiles is the stock GT86 layout. Thresholds live in ui.c's ranges:
// retune gauges there.
extern const tile_t ui_tiles[];
extern const int ui_tile_count;


// ui_render draws the frame for screen into c: the settings page, the
// data source's status while it has one, a single gauge, or every gauge
// in a grid. now_ms drives the spinner and staleness.
void ui_render(canvas_t *c, const state_t *state, const screen_t *screen, const settings_t *cfg, uint32_t now_ms);

// The boot animation: the logo revealed behind a scan line, the shift
// light revving to a blue flash, then every gauge sweeping up and back.
#define BOOT_REVEAL_MS 900
#define BOOT_SPLASH_MS 1600
#define BOOT_SWEEP_MS 2000
#define BOOT_DURATION_MS (BOOT_SPLASH_MS + BOOT_SWEEP_MS)

// ui_render_boot draws the boot animation elapsed_ms into it: the logo
// fades in and out, then every gauge sweeps up to its max and back down,
// like a car cluster's self-test at ignition. It returns false, without
// drawing, once BOOT_DURATION_MS is over.
bool ui_render_boot(canvas_t *c, uint32_t elapsed_ms, uint32_t now_ms);

// ui_wrap splits s into lines that fit max_width pixels in font, breaking
// between words, into out (at most max lines). A word wider
// than max_width gets a line of its own. It returns the line count.
int ui_wrap(const font_t *font, const char *s, int max_width, char out[][64], int max);
