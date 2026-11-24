#include "check.h"
#include "data/live.h"

// The live frame the Android app decodes: its bytes pinned here and in
// the app's LiveTest.kt, the same.
void test_live(void) {
    state_t s = {0};
    uint32_t now = 1000;
    state_set_reading(&s, SIG_OIL, 104.5f, now);
    state_set_reading(&s, SIG_COOLANT, 91, now);
    state_set_reading(&s, SIG_IAT, 31, now);
    state_set_reading(&s, SIG_AFR, 13.1f, now);
    state_set_reading(&s, SIG_BATTERY, 14.2f, now);
    state_set_reading(&s, SIG_ETHANOL, 68, now);
    // 3rd at 100 km/h (test_gear.c).
    state_set_reading(&s, SIG_RPM, 5361, now);
    state_set_reading(&s, SIG_SPEED, 100, now);
    state_set_reading(&s, SIG_AMBIENT, 14, now);
    state_set_drive_mode(&s, DRIVE_TRACK, now);
    s.trip_km = 12.34f;

    uint8_t out[LIVE_SIZE];
    live_encode(&s, now, out);
    const uint8_t want[LIVE_SIZE] = {0x01, 0x15, 0x04, 0x8E, 0x03, 0x1F, 0x83, 0x8E, 0x44,
                                     0xF1, 0x14, 0x03, 0x0E, 0x7B, 0x00, 0x00, 0x13};
    for (int i = 0; i < LIVE_SIZE; i++) {
        CHECK(out[i] == want[i], "live byte %d: %02X, want %02X", i, out[i], want[i]);
    }

    // Twenty seconds later nothing's fresh: every reading none, comfort,
    // still the trip.
    live_encode(&s, now + 20000, out);
    const uint8_t stale[LIVE_SIZE] = {0x01, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0xFF,
                                      0xFF, 0xFF, 0x00, 0x80, 0x7B, 0x00, 0x00, 0x10};
    for (int i = 0; i < LIVE_SIZE; i++) {
        CHECK(out[i] == stale[i], "stale live byte %d: %02X, want %02X", i, out[i], stale[i]);
    }
}
