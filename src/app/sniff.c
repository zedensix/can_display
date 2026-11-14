#include "sniff.h"

#include <stdio.h>
#include <string.h>

static bool same(uint8_t dlc_a, const uint8_t *a, uint8_t dlc_b, const uint8_t *b) {
    return dlc_a == dlc_b && memcmp(a, b, dlc_a) == 0;
}

void sniff_feed(sniff_t *s, const can_frame_t *f) {
    if (f->dlc > 8) {
        return;
    }
    sniff_id_t *e = NULL;
    for (int i = 0; i < s->count && !e; i++) {
        if (s->ids[i].id == f->id) {
            e = &s->ids[i];
        }
    }
    if (!e) {
        if (s->count == SNIFF_MAX_IDS) {
            return;
        }
        e = &s->ids[s->count++];
        *e = (sniff_id_t){.id = (uint16_t)f->id, .dlc = f->dlc, .before_dlc = f->dlc};
        memcpy(e->data, f->data, f->dlc);
        memcpy(e->before, f->data, f->dlc);
        return;
    }
    if (same(e->dlc, e->data, f->dlc, f->data)) {
        return;
    }
    if (!s->baseline_over) {
        e->noisy = true;
    } else if (!e->noisy) {
        if (e->changes == 0) {
            e->after_dlc = f->dlc;
            memcpy(e->after, f->data, f->dlc);
        }
        if (e->changes < UINT8_MAX) {
            e->changes++;
        }
    }
    e->dlc = f->dlc;
    memcpy(e->data, f->data, f->dlc);
}

void sniff_end_baseline(sniff_t *s) {
    s->baseline_over = true;
    for (int i = 0; i < s->count; i++) {
        sniff_id_t *e = &s->ids[i];
        e->before_dlc = e->dlc;
        memcpy(e->before, e->data, e->dlc);
    }
}

static void hex(char *out, const uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        snprintf(out + 2 * i, 3, "%02X", data[i]);
    }
}

void sniff_report(const sniff_t *s, void (*log)(void *ctx, const char *line), void *ctx) {
    int noisy = 0, candidates = 0;
    for (int i = 0; i < s->count; i++) {
        noisy += s->ids[i].noisy;
        candidates += !s->ids[i].noisy && s->ids[i].changes > 0;
    }
    char line[96];
    snprintf(line, sizeof line, "sniff: %d IDs seen, %d noisy, %d candidates", s->count, noisy, candidates);
    log(ctx, line);
    int logged = 0;
    for (int i = 0; i < s->count && logged < SNIFF_REPORTED; i++) {
        const sniff_id_t *e = &s->ids[i];
        if (e->noisy || e->changes == 0) {
            continue;
        }
        char before[17] = "", after[17] = "";
        hex(before, e->before, e->before_dlc);
        hex(after, e->after, e->after_dlc);
        snprintf(line, sizeof line, "sniff: %03X x%u: %s -> %s", e->id, e->changes, before, after);
        log(ctx, line);
        logged++;
    }
}
