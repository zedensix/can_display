#include "nmea.h"

#include <stdlib.h>
#include <string.h>

static int hex_value(char c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

static bool checksum_ok(const char *line) {
    const char *star = strchr(line, '*');
    if (line[0] != '$' || !star || hex_value(star[1]) < 0 || hex_value(star[2]) < 0) {
        return false;
    }
    uint8_t sum = 0;
    for (const char *p = line + 1; p < star; p++) {
        sum ^= (uint8_t)*p;
    }
    return sum == (hex_value(star[1]) << 4 | hex_value(star[2]));
}

// field returns the start of the n-th comma-separated field (0 is the
// sentence name), or NULL.
static const char *field(const char *line, int n) {
    for (; n > 0; n--) {
        line = strchr(line, ',');
        if (!line) {
            return NULL;
        }
        line++;
    }
    return line;
}

// coordinate turns NMEA's (d)ddmm.mmmm and its hemisphere into degrees
// x 1e7, with integers only past the decimal point.
static bool coordinate(const char *f, const char *hemisphere, int32_t *out) {
    const char *dot = strchr(f, '.');
    if (!dot || dot - f < 3 || !hemisphere) {
        return false;
    }
    int64_t degrees = 0;
    for (const char *p = f; p < dot - 2; p++) {
        degrees = degrees * 10 + (*p - '0');
    }
    // Minutes, as minutes x 1e5.
    int64_t minutes = (dot[-2] - '0') * 10 + (dot[-1] - '0');
    int64_t fraction = 0, scale = 1;
    for (const char *p = dot + 1; *p >= '0' && *p <= '9' && scale < 100000; p++) {
        fraction = fraction * 10 + (*p - '0');
        scale *= 10;
    }
    minutes = minutes * 100000 + fraction * (100000 / scale);
    int64_t value = degrees * 10000000 + minutes * 100 / 60;
    *out = (int32_t)(*hemisphere == 'S' || *hemisphere == 'W' ? -value : value);
    return true;
}

bool nmea_parse_rmc(const char *line, gps_fix_t *fix) {
    if ((strncmp(line, "$GPRMC,", 7) != 0 && strncmp(line, "$GNRMC,", 7) != 0) || !checksum_ok(line)) {
        return false;
    }
    *fix = (gps_fix_t){0};
    const char *time = field(line, 1), *status = field(line, 2);
    if (!time || !status) {
        return false;
    }
    if (*status != 'A') {
        return true;
    }
    if (strlen(time) < 6) {
        return true;
    }
    int hh = (time[0] - '0') * 10 + (time[1] - '0'), mm = (time[2] - '0') * 10 + (time[3] - '0');
    float ss = strtof(time + 4, NULL);
    fix->time_ms = (uint32_t)((hh * 3600 + mm * 60) * 1000) + (uint32_t)(ss * 1000 + 0.5f);
    const char *lat = field(line, 3), *lon = field(line, 5);
    if (!coordinate(lat, field(line, 4), &fix->lat) || !coordinate(lon, field(line, 6), &fix->lon)) {
        return true;
    }
    const char *speed = field(line, 7), *course = field(line, 8);
    fix->speed_kmh = speed ? strtof(speed, NULL) * 1.852f : 0;
    fix->course_deg = course ? strtof(course, NULL) : 0;
    fix->valid = true;
    return true;
}
