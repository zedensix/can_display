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
// (Send log, Scan ECU RAM, Listen CAN, Disconnect) return
// to the gauges and set action. Back returns to the gauges. With the CAN
// source, the rows that need the dongle (screen_row_needs_ble) do nothing.

typedef enum {
    PAGE_GAUGES,
    PAGE_SETTINGS,
} page_t;

// Rows of the settings page, top to bottom, in its groups: display,
// connection, tools, then Back.
enum {
    ROW_BRIGHTNESS,
    ROW_SHIFT_LIGHT,
    ROW_FLIP,
    ROW_SOURCE,
    ROW_AUTO_CONNECT,
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

// screen_apply_turn applies encoder detents, positive clockwise. views is how
// many gauges the single-gauge views cycle through.
void screen_apply_turn(screen_t *s, int detents, int views, settings_t *cfg);

// screen_apply_click applies a click.
void screen_apply_click(screen_t *s, settings_t *cfg);

// screen_row_needs_ble tells the settings rows that only work with the
// BLE dongle as the source (the radio is off with the CAN source).
bool screen_row_needs_ble(int row);
