#pragma once

#include <stdbool.h>
#include <stdint.h>

// settings_t is what the settings page changes. The firmware keeps it in
// a flash sector of its own.
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
} settings_t;

#define BRIGHTNESS_MIN 10
#define BRIGHTNESS_MAX 100
#define BRIGHTNESS_STEP 10

// SETTINGS_DEFAULT is what a Pico that never saved settings uses.
#define SETTINGS_DEFAULT ((settings_t){.brightness = BRIGHTNESS_MAX})

// SETTINGS_SIZE is how many bytes settings_encode writes.
#define SETTINGS_SIZE 7

void settings_encode(settings_t s, uint8_t out[SETTINGS_SIZE]);

// settings_decode unpacks what settings_encode wrote. Anything else
// (erased flash, another format) gives SETTINGS_DEFAULT.
settings_t settings_decode(const uint8_t in[SETTINGS_SIZE]);

bool settings_equal(settings_t a, settings_t b);
