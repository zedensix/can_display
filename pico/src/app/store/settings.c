#include "settings.h"

#include <string.h>

// "GT86" then a format version mark a sector holding settings.
static const uint8_t magic[4] = {'G', 'T', '8', '6'};
// Older versions still decode: version 1 without the shift light (off).
// Versions 3 to 5 went on past byte 7 (a lap timer's line, then the BLE
// dongle last connected to): unused since, like their flags.
#define VERSION 6

enum {
    // 1 << 0 was auto-connect, 1 << 4 a dongle saved, 1 << 5 the CAN
    // source (the BLE dongle without it): the MCP2515 is the only source.
    FLAG_FLIP = 1 << 1,
    // 1 << 2 was an analog ethanol sensor, dropped: the ECU's RAM has it.
    // 1 << 6 was landscape: the panel is always in landscape now.
};

void settings_encode(settings_t s, uint8_t out[SETTINGS_SIZE]) {
    memcpy(out, magic, sizeof magic);
    out[4] = VERSION;
    out[5] = (uint8_t)s.brightness;
    out[6] = (uint8_t)(s.flip ? FLAG_FLIP : 0);
    out[7] = (uint8_t)(s.shift_rpm / SHIFT_RPM_STEP);
}

settings_t settings_decode(const uint8_t in[SETTINGS_SIZE]) {
    if (memcmp(in, magic, sizeof magic) != 0 || in[4] < 1 || in[4] > VERSION || in[5] < BRIGHTNESS_MIN ||
        in[5] > BRIGHTNESS_MAX) {
        return SETTINGS_DEFAULT;
    }
    int shift = in[4] >= 2 ? in[7] * SHIFT_RPM_STEP : 0;
    return (settings_t){
        .brightness = in[5],
        .flip = (in[6] & FLAG_FLIP) != 0,
        .shift_rpm = shift >= SHIFT_RPM_MIN && shift <= SHIFT_RPM_MAX ? shift : 0,
    };
}

bool settings_equal(settings_t a, settings_t b) {
    return a.brightness == b.brightness && a.flip == b.flip && a.shift_rpm == b.shift_rpm;
}
