#include "laptimer.h"

#include <math.h>

#define DAY_MS 86400000u
#define PI 3.14159265f

lap_line_t lap_line_at(const gps_fix_t *fix) {
    lap_line_t line = {.set = true, .lat = fix->lat, .lon = fix->lon, .heading = -1};
    lap_line_settle(&line, fix);
    return line;
}

bool lap_line_settle(lap_line_t *line, const gps_fix_t *fix) {
    if (!line->set || line->heading >= 0 || !fix->valid || fix->speed_kmh < LAP_MOVING_KMH) {
        return false;
    }
    line->heading = (int16_t)((int)lroundf(fix->course_deg) % 360);
    return true;
}

// along and across place fix relative to the line, in meters: along the
// crossing direction, and across it.
static void place(const lap_line_t *line, const gps_fix_t *fix, float *along, float *across) {
    float north = (float)(fix->lat - line->lat) * 1e-7f * 110574;
    float east = (float)(fix->lon - line->lon) * 1e-7f * 111320 * cosf((float)line->lat * 1e-7f * PI / 180);
    float h = (float)line->heading * PI / 180;
    *along = east * sinf(h) + north * cosf(h);
    *across = east * cosf(h) - north * sinf(h);
}

bool laptimer_feed(laptimer_t *l, const lap_line_t *line, const gps_fix_t *fix, uint32_t now_ms) {
    if (!fix->valid) {
        return false;
    }
    gps_fix_t prev = l->prev;
    l->prev = *fix;
    if (!line->set || line->heading < 0 || !prev.valid || fix->speed_kmh < LAP_MOVING_KMH) {
        return false;
    }
    float a0, c0, a1, c1;
    place(line, &prev, &a0, &c0);
    place(line, fix, &a1, &c1);
    if (!(a0 < 0 && a1 >= 0) || a1 - a0 > 100) {
        return false;
    }
    float f = -a0 / (a1 - a0);
    float across = c0 + f * (c1 - c0);
    if (across < -LAP_GATE_HALF_WIDTH_M || across > LAP_GATE_HALF_WIDTH_M) {
        return false;
    }
    uint32_t dt = (fix->time_ms + DAY_MS - prev.time_ms) % DAY_MS;
    uint32_t crossing = (prev.time_ms + (uint32_t)lroundf(f * (float)dt)) % DAY_MS;
    uint32_t lap = (crossing + DAY_MS - l->start_gps_ms) % DAY_MS;
    if (l->timing && lap < LAP_MIN_MS) {
        return false;
    }
    // The same moment on the Pico's clock: the fix arrived now.
    uint32_t since = (fix->time_ms + DAY_MS - crossing) % DAY_MS;
    bool ended = l->timing;
    if (ended) {
        l->laps++;
        l->last_ms = lap;
        if (l->best_ms == 0 || lap < l->best_ms) {
            l->best_ms = lap;
        }
        l->completed_at_ms = now_ms;
    }
    l->timing = true;
    l->start_gps_ms = crossing;
    l->start_at_ms = now_ms - since;
    return ended;
}

uint32_t laptimer_running_ms(const laptimer_t *l, uint32_t now_ms) {
    return l->timing ? now_ms - l->start_at_ms : 0;
}
