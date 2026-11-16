#include "sniff.h"

#include <stdio.h>
#include <string.h>

const sniff_step_t sniff_steps[SNIFF_STEPS] = {
    {"everything on:\nno lamp lit", SNIFF_ALL_ON},
    {"press TRC OFF briefly:\nTRC OFF lit", SNIFF_TRC_OFF},
    {"press TRC OFF again:\nlamp out", SNIFF_ALL_ON},
    {"press VSC SPORT:\nSPORT lit", SNIFF_SPORT},
    {"press TRC OFF briefly\ntoo: both lit", SNIFF_SPORT_TRC_OFF},
    {"VSC SPORT and TRC OFF\nagain: lamps out", SNIFF_ALL_ON},
    {"hold TRC OFF a few\nseconds: VSC OFF lit", SNIFF_ALL_OFF},
    {"press TRC OFF again:\nlamps out", SNIFF_ALL_ON},
};

static const char *const mode_names[SNIFF_MODES] = {"on", "trc", "sport", "sport+trc", "off"};

void sniff_add_frame(sniff_t *s, const can_frame_t *f) {
    if (f->dlc > 8 || s->step < 0 || s->step >= SNIFF_STEPS) {
        return;
    }
    uint8_t data[8] = {0};
    memcpy(data, f->data, f->dlc);
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
        *e = (sniff_id_t){.id = (uint16_t)f->id};
    }
    uint8_t bit = (uint8_t)(1u << s->step);
    if (e->seen & bit) {
        for (int i = 0; i < 8; i++) {
            e->noise[i] |= e->data[s->step][i] ^ data[i];
        }
    }
    e->seen |= bit;
    e->dlc = f->dlc;
    memcpy(e->data[s->step], data, 8);
}

// candidate_bits sets bits to e's bits that tell the modes apart: quiet
// within each step, the same in every step of a mode, different between
// two modes. It returns whether there's any.
static bool candidate_bits(const sniff_id_t *e, uint8_t bits[8]) {
    bool any = false;
    for (int b = 0; b < 8; b++) {
        uint8_t across = 0, within = 0;
        for (int i = 0; i < SNIFF_STEPS; i++) {
            for (int j = i + 1; j < SNIFF_STEPS; j++) {
                uint8_t diff = e->data[i][b] ^ e->data[j][b];
                if (sniff_steps[i].mode == sniff_steps[j].mode) {
                    within |= diff;
                } else {
                    across |= diff;
                }
            }
        }
        bits[b] = across & (uint8_t)~within & (uint8_t)~e->noise[b];
        any |= bits[b] != 0;
    }
    return any;
}

static void bytes_to_hex(char *out, const uint8_t *data, int len) {
    out[0] = 0;
    for (int i = 0; i < len; i++) {
        snprintf(out + 2 * i, 3, "%02X", data[i]);
    }
}

void sniff_log_candidates(const sniff_t *s, void (*log)(void *ctx, const char *line), void *ctx) {
    const uint8_t every_step = (1u << SNIFF_STEPS) - 1;
    int complete = 0, noisy = 0, candidates = 0;
    uint8_t bits[8];
    for (int i = 0; i < s->count; i++) {
        const sniff_id_t *e = &s->ids[i];
        complete += e->seen == every_step;
        for (int b = 0; b < 8; b++) {
            if (e->noise[b]) {
                noisy++;
                break;
            }
        }
        candidates += e->seen == every_step && candidate_bits(e, bits);
    }
    char line[200];
    snprintf(line, sizeof line, "sniff: %d IDs seen, %d in every step, %d with noise, %d candidates", s->count,
             complete, noisy, candidates);
    log(ctx, line);
    int logged = 0;
    for (int i = 0; i < s->count && logged < SNIFF_REPORTED; i++) {
        const sniff_id_t *e = &s->ids[i];
        if (e->seen != every_step || !candidate_bits(e, bits)) {
            continue;
        }
        char mask[17];
        bytes_to_hex(mask, bits, e->dlc);
        int len = snprintf(line, sizeof line, "sniff: %03X bits %s:", e->id, mask);
        // Each mode's data from its first step.
        for (int m = 0; m < SNIFF_MODES; m++) {
            for (int k = 0; k < SNIFF_STEPS; k++) {
                if (sniff_steps[k].mode == (sniff_mode_t)m) {
                    char data[17];
                    bytes_to_hex(data, e->data[k], e->dlc);
                    len += snprintf(line + len, sizeof line - (size_t)len, " %s %s", mode_names[m], data);
                    break;
                }
            }
        }
        log(ctx, line);
        logged++;
    }
}
