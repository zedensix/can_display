#pragma once

#include <stdbool.h>

#include "settings.h"

// screen_t is what the rotary encoder drives (the simulator's keyboard
// stands in for it).
//
// On the gauges, turning cycles the views: every gauge, then each one
// alone. A click opens the settings. There, turning moves the cursor, a
// click toggles the row under it or, on brightness, makes turning change
// the value until the next click. Back returns to the gauges.

typedef enum {
    PAGE_GAUGES,
    PAGE_SETTINGS,
} page_t;

// Rows of the settings page, top to bottom.
enum {
    ROW_BRIGHTNESS,
    ROW_AUTO_CONNECT,
    ROW_FLIP,
    ROW_ETHANOL_SENSOR,
    ROW_BACK,
    SETTINGS_ROWS,
};

typedef struct {
    page_t page;
    // view is 0 for every gauge, k for the k-th gauge alone.
    int view;
    // cursor is the selected settings row.
    int cursor;
    // editing makes turning change the brightness.
    bool editing;
} screen_t;

// screen_turn applies encoder detents, positive clockwise. views is how
// many gauges the single-gauge views cycle through.
void screen_turn(screen_t *s, int detents, int views, settings_t *cfg);

// screen_press applies a click.
void screen_press(screen_t *s, settings_t *cfg);
