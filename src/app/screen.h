#pragma once

#include <stdbool.h>

#include "settings.h"

// screen_t is what the rotary encoder drives (the simulator's keyboard
// stands in for it).
//
// On the gauges, turning cycles the views: every gauge, then each one
// alone. A click opens the settings. There, turning moves the cursor, a
// click toggles the row under it or, on brightness and the shift light,
// makes turning change the value until the next click. The action rows
// (Set lap line, Send log, Scan ECU RAM, Listen CAN, Disconnect) return
// to the gauges and set action. Back returns to the gauges.

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
    ROW_LAP_LINE,
    ROW_SEND_LOG,
    ROW_SCAN,
    ROW_LISTEN,
    ROW_DISCONNECT,
    ROW_BACK,
    SETTINGS_ROWS,
};

// action_t is a click on an action row, for the caller to act on and
// clear.
typedef enum {
    ACTION_NONE,
    ACTION_SET_LAP_LINE,
    ACTION_SEND_LOG,
    ACTION_SCAN,
    ACTION_LISTEN,
    ACTION_DISCONNECT,
} action_t;

typedef struct {
    page_t page;
    // view is 0 for every gauge, k for the k-th gauge alone.
    int view;
    // cursor is the selected settings row.
    int cursor;
    // editing makes turning change the value of the row under the cursor.
    bool editing;
    action_t action;
} screen_t;

// screen_turn applies encoder detents, positive clockwise. views is how
// many gauges the single-gauge views cycle through.
void screen_turn(screen_t *s, int detents, int views, settings_t *cfg);

// screen_press applies a click.
void screen_press(screen_t *s, settings_t *cfg);
