#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "laptimer.h"

// settings_t is what the settings page changes. The firmware keeps it in
// a flash sector of its own.
// dongle_t is a BLE device's address, with its type (public or random)
// as BLE needs it to connect.
typedef struct {
    bool set;
    uint8_t addr[6];
    uint8_t type;
} dongle_t;

typedef struct {
    // brightness is the backlight level, in percent.
    int brightness;
    // auto_connect connects to the OBD dongle at power-on, and reconnects
    // by itself after a failure, instead of waiting for a click.
    bool auto_connect;
    // flip turns the picture upside down, for a panel mounted that way.
    bool flip;
    // ethanol_sensor reads the flex-fuel sensor. Off, the ADC pin isn't
    // read at all: left unconnected it floats, and shows noise as a
    // plausible ethanol %.
    bool ethanol_sensor;
    // shift_rpm flashes the screen's edge from that RPM on, like a shift
    // light; 0 is off.
    int shift_rpm;
    // lap_line is the lap timer's start/finish line.
    lap_line_t lap_line;
    // dongle is the OBD dongle last connected to: the connect prompt
    // offers it first.
    dongle_t dongle;
} settings_t;

#define BRIGHTNESS_MIN 10
#define BRIGHTNESS_MAX 100
#define BRIGHTNESS_STEP 10
#define SHIFT_RPM_MIN 5000
// The rev limiter (see RPM_RANGE in ui.c).
#define SHIFT_RPM_MAX 7600
#define SHIFT_RPM_STEP 100

// SETTINGS_DEFAULT is what a Pico that never saved settings uses.
#define SETTINGS_DEFAULT ((settings_t){.brightness = BRIGHTNESS_MAX})

// SETTINGS_SIZE is how many bytes settings_encode writes.
#define SETTINGS_SIZE 25

void settings_encode(settings_t s, uint8_t out[SETTINGS_SIZE]);

// settings_decode unpacks what settings_encode wrote. Anything else
// (erased flash, another format) gives SETTINGS_DEFAULT.
settings_t settings_decode(const uint8_t in[SETTINGS_SIZE]);

bool settings_equal(settings_t a, settings_t b);
bool dongle_equal(dongle_t a, dongle_t b);
