#include "settings.h"

#include <string.h>

// "GT86" then a format version mark a sector holding settings.
static const uint8_t magic[4] = {'G', 'T', '8', '6'};
#define VERSION 1

enum {
    FLAG_AUTO_CONNECT = 1 << 0,
    FLAG_FLIP = 1 << 1,
    FLAG_ETHANOL_SENSOR = 1 << 2,
};

void settings_encode(settings_t s, uint8_t out[SETTINGS_SIZE]) {
    memcpy(out, magic, sizeof magic);
    out[4] = VERSION;
    out[5] = (uint8_t)s.brightness;
    out[6] = (uint8_t)((s.auto_connect ? FLAG_AUTO_CONNECT : 0) | (s.flip ? FLAG_FLIP : 0) |
                       (s.ethanol_sensor ? FLAG_ETHANOL_SENSOR : 0));
}

settings_t settings_decode(const uint8_t in[SETTINGS_SIZE]) {
    if (memcmp(in, magic, sizeof magic) != 0 || in[4] != VERSION || in[5] < BRIGHTNESS_MIN ||
        in[5] > BRIGHTNESS_MAX) {
        return SETTINGS_DEFAULT;
    }
    return (settings_t){
        .brightness = in[5],
        .auto_connect = (in[6] & FLAG_AUTO_CONNECT) != 0,
        .flip = (in[6] & FLAG_FLIP) != 0,
        .ethanol_sensor = (in[6] & FLAG_ETHANOL_SENSOR) != 0,
    };
}

bool settings_equal(settings_t a, settings_t b) {
    return a.brightness == b.brightness && a.auto_connect == b.auto_connect && a.flip == b.flip &&
           a.ethanol_sensor == b.ethanol_sensor;
}
