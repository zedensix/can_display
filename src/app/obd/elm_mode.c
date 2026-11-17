// The broadcast frames read through the dongle, the drive mode and the
// doors: polling stops for a moment of monitoring each ID alone in turn
// (see ELM_MODE_*).

#include <stdio.h>

#include "elm_internal.h"

static const uint16_t monitor_ids[ELM_MONITORS] = {GT86_ID_DRIVE_MODE, GT86_ID_DOORS};

// The steps: 0 ATCAF0; then for monitor k, 1 + 2k ATCRA<id> and 2 + 2k
// ATMA; then ATCRA and ATCAF1.
#define STEP_ATCRA (1 + 2 * ELM_MONITORS)
#define STEP_ATCAF1 (STEP_ATCRA + 1)

// The doors only at a standstill: 0x375 comes too seldom for a short
// monitor, and a long one while driving would starve the gauges.
static bool monitor_wanted(const elm_t *e, int k) {
    const reading_t *speed = &e->state->readings[SIG_SPEED];
    return e->mode_misses[k] < ELM_MODE_MAX_MISSES && (k == 0 || (speed->valid && speed->value < 1));
}

bool mode_read_due(const elm_t *e, uint32_t now_ms) {
    bool any_wanted = false;
    for (int k = 0; k < ELM_MONITORS; k++) {
        any_wanted = any_wanted || monitor_wanted(e, k);
    }
    return any_wanted && now_ms - e->mode_ms >= ELM_MODE_INTERVAL_MS;
}

void send_next_mode_command(elm_t *e, uint32_t now_ms) {
    int step = e->mode_step;
    if (step == 0) {
        e->mode_ms = now_ms;
        e->mode_step = 1;
        send_command(e, "ATCAF0", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    // An ID given up is skipped, its ATCRA and its ATMA.
    while (step < STEP_ATCRA && step % 2 == 1 && !monitor_wanted(e, (step - 1) / 2)) {
        step += 2;
    }
    e->mode_step = step == STEP_ATCAF1 ? 0 : step + 1;
    if (step == STEP_ATCRA || step == STEP_ATCAF1) {
        send_command(e, step == STEP_ATCRA ? "ATCRA" : "ATCAF1", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    e->mode_monitor = (step - 1) / 2;
    if (step % 2 == 1) {
        char command[16];
        snprintf(command, sizeof command, "ATCRA%03X", monitor_ids[e->mode_monitor]);
        send_command(e, command, ELM_TIMEOUT_MS, now_ms);
        return;
    }
    e->mode_got = false;
    e->monitoring = true;
    e->stop_sent = false;
    e->monitor_ms = now_ms;
    e->monitor_dwell_ms = e->mode_monitor == 0 ? ELM_MODE_DWELL_MS : ELM_DOORS_DWELL_MS;
    send_command(e, "ATMA", e->monitor_dwell_ms + ELM_TIMEOUT_MS, now_ms);
}

bool mode_monitor_frame(elm_t *e, const can_frame_t *f, uint32_t now_ms) {
    bool was_read = e->state->drive_mode_read;
    drive_mode_t was = e->state->drive_mode;
    if (f->id != monitor_ids[e->mode_monitor] || !gt86_decode_broadcast(f, e->state, now_ms)) {
        return false;
    }
    e->mode_got = true;
    // A mode change is logged with its frame, to check the bits against
    // the cluster's lamps.
    if (f->id == GT86_ID_DRIVE_MODE && (!was_read || was != e->state->drive_mode)) {
        static const char *const names[DRIVE_MODES] = {"normal", "TRC off", "VSC sport", "sport + TRC off", "VSC off"};
        char line[80];
        int n = snprintf(line, sizeof line, "elm327: drive mode %s, 0D3", names[e->state->drive_mode]);
        for (int i = 0; i < f->dlc && n > 0 && n < (int)sizeof line - 3; i++) {
            n += snprintf(line + n, sizeof line - (size_t)n, " %02X", f->data[i]);
        }
        e->cb.log(e->cb.ctx, line);
    }
    return true;
}

void mode_monitor_over(elm_t *e) {
    e->monitoring = false;
    int k = e->mode_monitor;
    // Without the frame, it only counts against it while the ECU answers
    // the rest: with the car off, nothing's on the bus.
    if (e->mode_got) {
        e->mode_misses[k] = 0;
    } else if (e->ecu_answered && ++e->mode_misses[k] == ELM_MODE_MAX_MISSES) {
        char line[64];
        snprintf(line, sizeof line, "elm327: no %03X frame on the bus, giving up", monitor_ids[k]);
        e->cb.log(e->cb.ctx, line);
    }
}
