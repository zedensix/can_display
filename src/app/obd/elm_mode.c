// The drive mode, read through the dongle: a broadcast frame, so polling
// stops for a moment of monitoring that one ID (see ELM_MODE_*).

#include <stdio.h>

#include "elm_internal.h"

static const char *const mode_commands[] = {"ATCAF0", "ATCRA0D3", "ATMA", "ATCRA", "ATCAF1"};
#define MODE_COMMANDS (int)(sizeof mode_commands / sizeof mode_commands[0])
#define MODE_MONITOR 2 // mode_commands' ATMA

bool mode_read_due(const elm_t *e, uint32_t now_ms) {
    return e->mode_misses < ELM_MODE_MAX_MISSES && now_ms - e->mode_ms >= ELM_MODE_INTERVAL_MS;
}

void send_next_mode_command(elm_t *e, uint32_t now_ms) {
    if (e->mode_step == 0) {
        e->mode_ms = now_ms;
    }
    int step = e->mode_step;
    e->mode_step = (step + 1) % MODE_COMMANDS;
    if (step == MODE_MONITOR) {
        e->mode_got = false;
        e->monitoring = true;
        e->stop_sent = false;
        e->monitor_ms = now_ms;
        e->monitor_dwell_ms = ELM_MODE_DWELL_MS;
        send_command(e, mode_commands[step], ELM_MODE_DWELL_MS + ELM_TIMEOUT_MS, now_ms);
        return;
    }
    send_command(e, mode_commands[step], ELM_TIMEOUT_MS, now_ms);
}

bool mode_monitor_frame(elm_t *e, const can_frame_t *f, uint32_t now_ms) {
    if (f->id != GT86_ID_DRIVE_MODE || !gt86_decode_broadcast(f, e->state, now_ms)) {
        return false;
    }
    e->mode_got = true;
    return true;
}

void mode_monitor_over(elm_t *e) {
    e->monitoring = false;
    // Without the frame, it only counts against it while the ECU answers
    // the rest: with the car off, nothing's on the bus.
    if (e->mode_got) {
        e->mode_misses = 0;
    } else if (e->ecu_answered && ++e->mode_misses == ELM_MODE_MAX_MISSES) {
        e->cb.log(e->cb.ctx, "elm327: no drive mode frame (0D3) on the bus, giving up");
    }
}
