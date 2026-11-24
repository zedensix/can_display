#include "check.h"
#include "data/signals.h"
#include "obd/obd.h"

// The gear from RPM and speed, the trip added up from the speed, and the
// outside temperature and odometer PIDs.
void test_gear_and_trip(void) {
    state_t s = {0};
    CHECK(state_gear(&s, 0, 2000) == 0, "a gear without readings");
    // 3rd at 100 km/h: 100 x 34.79 x 1.541 = 5361 rpm.
    state_set_reading(&s, SIG_RPM, 5361, 1000);
    state_set_reading(&s, SIG_SPEED, 100, 1000);
    CHECK(state_gear(&s, 1000, 2000) == 3, "5361 rpm at 100 km/h gave gear %d, want 3", state_gear(&s, 1000, 2000));
    // 6th at 130: 3469 rpm; 5% off still counts.
    state_set_reading(&s, SIG_RPM, 3469 * 1.05f, 1000);
    state_set_reading(&s, SIG_SPEED, 130, 1000);
    CHECK(state_gear(&s, 1000, 2000) == 6, "6th gave gear %d", state_gear(&s, 1000, 2000));
    // The clutch in: idling at 60 km/h, between no gear's.
    state_set_reading(&s, SIG_RPM, 900, 1000);
    state_set_reading(&s, SIG_SPEED, 60, 1000);
    CHECK(state_gear(&s, 1000, 2000) == 0, "idling at 60 km/h gave gear %d", state_gear(&s, 1000, 2000));
    CHECK(state_gear(&s, 4000, 2000) == 0, "stale readings gave a gear");

    // 90 km/h for 10s, a reading every 200ms: 250m. A gap past
    // TRIP_MAX_GAP_MS adds nothing.
    state_t t = {0};
    for (uint32_t ms = 0; ms <= 10000; ms += 200) {
        state_set_reading(&t, SIG_SPEED, 90, ms);
    }
    CHECK(fabsf(t.trip_km - 0.25f) < 0.001f, "trip %f km, want 0.25", (double)t.trip_km);
    state_set_reading(&t, SIG_SPEED, 90, 10000 + TRIP_MAX_GAP_MS + 1);
    CHECK(fabsf(t.trip_km - 0.25f) < 0.001f, "a gap added to the trip: %f km", (double)t.trip_km);

    // 41 46 37: 15°C outside; 41 A6 00 01 E2 40: 12345.6 km.
    state_t u = {0};
    CHECK(obd_decode_message((const uint8_t[]){0x41, 0x46, 0x37}, 3, &u, 0) == SIG_AMBIENT &&
              nearly_equal(u.readings[SIG_AMBIENT].value, 15),
          "outside %f, want 15", (double)u.readings[SIG_AMBIENT].value);
    CHECK(obd_decode_message((const uint8_t[]){0x41, 0xA6, 0x00, 0x01, 0xE2, 0x40}, 6, &u, 0) == SIG_ODOMETER &&
              fabsf(u.readings[SIG_ODOMETER].value - 12345.6f) < 0.01f,
          "odometer %f, want 12345.6", (double)u.readings[SIG_ODOMETER].value);
}
