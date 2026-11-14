#pragma once

#include <stdbool.h>

#include "settings.h"

// screen_t is what the rotary encoder drives (the simulator's keyboard
// stands in for it).
//
// On the gauges, turning cycles the views: every gauge, then each one
// alone. A click opens the settings. There, turning moves the cursor, a
// click toggles the row under it or, on brightness and the shift light,
// makes turning change the value until the next click. Scan ECU RAM,
// Listen CAN and Disconnect return to the gauges and raise
// scan_requested, listen_requested or disconnect_requested. Back returns
// to the gauges.

typedef enum {
    PAGE_GAUGES,
    PAGE_SETTINGS,
} page_t;

// Rows of the settings page, top to bottom.
enum {
    ROW_BRIGHTNESS,
    ROW_SHIFT_LIGHT,
    ROW_AUTO_CONNECT,
    ROW_FLIP,
    ROW_ETHANOL_SENSOR,
    ROW_SCAN,
    ROW_LISTEN,
    ROW_DISCONNECT,
    ROW_BACK,
    SETTINGS_ROWS,
};

typedef struct {
    page_t page;
    // view is 0 for every gauge, k for the k-th gauge alone.
    int view;
    // cursor is the selected settings row.
    int cursor;
    // editing makes turning change the value of the row under the cursor.
    bool editing;
    // The clicks on Scan ECU RAM, Listen CAN and Disconnect, for the
    // caller to act on and clear.
    bool scan_requested, listen_requested, disconnect_requested;
} screen_t;

// screen_turn applies encoder detents, positive clockwise. views is how
// many gauges the single-gauge views cycle through.
void screen_turn(screen_t *s, int detents, int views, settings_t *cfg);

// screen_press applies a click.
void screen_press(screen_t *s, settings_t *cfg);
