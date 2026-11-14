#pragma once

#include <stdbool.h>
#include <stdint.h>

// gps_fix_t is one GPS position, from an NMEA RMC sentence.
typedef struct {
    bool valid;
    // time_ms is the fix's UTC time of day: lap times come from it, it
    // doesn't drift nor wait on the serial line like the Pico's clock.
    uint32_t time_ms;
    // Degrees x 1e7 (about 1cm): floats would lose a meter.
    int32_t lat, lon;
    float speed_kmh, course_deg;
} gps_fix_t;

// nmea_parse_rmc parses one sentence ("$GPRMC,..." or "$GNRMC,...", "*"
// checksum included, no line ending) into fix. It returns false for
// other sentences or a bad checksum; a void fix (status V) parses with
// valid false.
bool nmea_parse_rmc(const char *line, gps_fix_t *fix);
