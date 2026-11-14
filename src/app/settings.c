#include "settings.h"

#include <string.h>

// "GT86" then a format version mark a sector holding settings.
static const uint8_t magic[4] = {'G', 'T', '8', '6'};
// Older versions still decode: version 1 without the shift light (off),
// version 2 without the lap line (unset).
#define VERSION 3

enum {
    FLAG_AUTO_CONNECT = 1 << 0,
    FLAG_FLIP = 1 << 1,
    FLAG_ETHANOL_SENSOR = 1 << 2,
    FLAG_LAP_LINE = 1 << 3,
};

static void put32(uint8_t *p, uint32_t v) {
    for (int i = 0; i < 4; i++) {
        p[i] = (uint8_t)(v >> (8 * i));
    }
}

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

void settings_encode(settings_t s, uint8_t out[SETTINGS_SIZE]) {
    memcpy(out, magic, sizeof magic);
    out[4] = VERSION;
    out[5] = (uint8_t)s.brightness;
    out[6] = (uint8_t)((s.auto_connect ? FLAG_AUTO_CONNECT : 0) | (s.flip ? FLAG_FLIP : 0) |
                       (s.ethanol_sensor ? FLAG_ETHANOL_SENSOR : 0) | (s.lap_line.set ? FLAG_LAP_LINE : 0));
    out[7] = (uint8_t)(s.shift_rpm / SHIFT_RPM_STEP);
    put32(out + 8, (uint32_t)s.lap_line.lat);
    put32(out + 12, (uint32_t)s.lap_line.lon);
    out[16] = (uint8_t)s.lap_line.heading;
    out[17] = (uint8_t)((uint16_t)s.lap_line.heading >> 8);
}

settings_t settings_decode(const uint8_t in[SETTINGS_SIZE]) {
    if (memcmp(in, magic, sizeof magic) != 0 || in[4] < 1 || in[4] > VERSION || in[5] < BRIGHTNESS_MIN ||
        in[5] > BRIGHTNESS_MAX) {
        return SETTINGS_DEFAULT;
    }
    int shift = in[4] >= 2 ? in[7] * SHIFT_RPM_STEP : 0;
    return (settings_t){
        .brightness = in[5],
        .auto_connect = (in[6] & FLAG_AUTO_CONNECT) != 0,
        .flip = (in[6] & FLAG_FLIP) != 0,
        .ethanol_sensor = (in[6] & FLAG_ETHANOL_SENSOR) != 0,
        .shift_rpm = shift >= SHIFT_RPM_MIN && shift <= SHIFT_RPM_MAX ? shift : 0,
        .lap_line =
            in[4] >= 3 && (in[6] & FLAG_LAP_LINE)
                ? (lap_line_t){true, (int32_t)get32(in + 8), (int32_t)get32(in + 12), (int16_t)(in[16] | in[17] << 8)}
                : (lap_line_t){0},
    };
}

bool settings_equal(settings_t a, settings_t b) {
    return a.brightness == b.brightness && a.auto_connect == b.auto_connect && a.flip == b.flip &&
           a.ethanol_sensor == b.ethanol_sensor && a.shift_rpm == b.shift_rpm &&
           a.lap_line.set == b.lap_line.set && a.lap_line.lat == b.lap_line.lat && a.lap_line.lon == b.lap_line.lon &&
           a.lap_line.heading == b.lap_line.heading;
}
