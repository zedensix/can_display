#include "settings.h"

#include <string.h>

// "GT86" then a format version mark a sector holding settings.
static const uint8_t magic[4] = {'G', 'T', '8', '6'};
// Older versions still decode: version 1 without the shift light (off),
// version 3 without the dongle, version 4 without its name. Bytes 8 to
// 17 held a lap timer's line (versions 3 to 5): unused since.
#define VERSION 5

enum {
    FLAG_AUTO_CONNECT = 1 << 0,
    FLAG_FLIP = 1 << 1,
    FLAG_ETHANOL_SENSOR = 1 << 2,
    FLAG_DONGLE = 1 << 4,
    // Clear in settings saved before the choice existed: the dongle.
    FLAG_CAN_SOURCE = 1 << 5,
};

void settings_encode(settings_t s, uint8_t out[SETTINGS_SIZE]) {
    memcpy(out, magic, sizeof magic);
    out[4] = VERSION;
    out[5] = (uint8_t)s.brightness;
    out[6] = (uint8_t)((s.auto_connect ? FLAG_AUTO_CONNECT : 0) | (s.flip ? FLAG_FLIP : 0) |
                       (s.ethanol_sensor ? FLAG_ETHANOL_SENSOR : 0) | (s.dongle.set ? FLAG_DONGLE : 0) |
                       (s.source == SOURCE_CAN ? FLAG_CAN_SOURCE : 0));
    out[7] = (uint8_t)(s.shift_rpm / SHIFT_RPM_STEP);
    memset(out + 8, 0, 10);
    memcpy(out + 18, s.dongle.addr, sizeof s.dongle.addr);
    out[24] = s.dongle.type;
    memcpy(out + 25, s.dongle.name, DONGLE_NAME);
}

settings_t settings_decode(const uint8_t in[SETTINGS_SIZE]) {
    if (memcmp(in, magic, sizeof magic) != 0 || in[4] < 1 || in[4] > VERSION || in[5] < BRIGHTNESS_MIN ||
        in[5] > BRIGHTNESS_MAX) {
        return SETTINGS_DEFAULT;
    }
    int shift = in[4] >= 2 ? in[7] * SHIFT_RPM_STEP : 0;
    dongle_t dongle = {0};
    if (in[4] >= 4 && (in[6] & FLAG_DONGLE)) {
        dongle.set = true;
        memcpy(dongle.addr, in + 18, sizeof dongle.addr);
        dongle.type = in[24];
        if (in[4] >= 5) {
            memcpy(dongle.name, in + 25, DONGLE_NAME);
            dongle.name[DONGLE_NAME - 1] = 0;
        }
    }
    return (settings_t){
        .source = (in[6] & FLAG_CAN_SOURCE) ? SOURCE_CAN : SOURCE_BLE,
        .brightness = in[5],
        .auto_connect = (in[6] & FLAG_AUTO_CONNECT) != 0,
        .flip = (in[6] & FLAG_FLIP) != 0,
        .ethanol_sensor = (in[6] & FLAG_ETHANOL_SENSOR) != 0,
        .shift_rpm = shift >= SHIFT_RPM_MIN && shift <= SHIFT_RPM_MAX ? shift : 0,
        .dongle = dongle,
    };
}

bool settings_equal(settings_t a, settings_t b) {
    return a.source == b.source && a.brightness == b.brightness && a.auto_connect == b.auto_connect &&
           a.flip == b.flip && a.ethanol_sensor == b.ethanol_sensor && a.shift_rpm == b.shift_rpm &&
           dongle_equal(a.dongle, b.dongle);
}

bool dongle_equal(dongle_t a, dongle_t b) {
    return a.set == b.set &&
           (!a.set || (memcmp(a.addr, b.addr, sizeof a.addr) == 0 && a.type == b.type && strcmp(a.name, b.name) == 0));
}
