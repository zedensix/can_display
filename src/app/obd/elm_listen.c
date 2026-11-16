#include <stdio.h>
#include <string.h>

#include "elm_internal.h"

// Listening sets the dongle to print raw frames (ATCAF0) through an ID
// mask of 0x700 (ATCM700), then, for each of sniff_steps, prompts the
// driver and waits for a click, then moves the window (ATCF) and monitors
// (ATMA) in turn, ELM_LISTEN_ROUNDS times over the bus. Any character
// stops a monitor; the dongle stops by itself, with BUFFER FULL, when BLE
// can't keep up. At the end, the session starts over (ATZ) rather than
// undo all that.
void start_can_listen(elm_t *e, uint32_t now_ms) {
    e->listen_requested = false;
    e->phase = ELM_LISTEN;
    memset(&e->sniff, 0, sizeof e->sniff);
    e->cb.log(e->cb.ctx, "sniff: listening");
    state_set_status(e->state, "LISTEN\nsetting the dongle up");
    send_command(e, "ATCAF0", ELM_TIMEOUT_MS, now_ms);
}

static void prompt_listen_step(elm_t *e) {
    e->listen_awaiting_click = true;
    char status[128];
    snprintf(status, sizeof status, "LISTEN %d/%d\n%s\nthen click", e->sniff.step + 1, SNIFF_STEPS,
             sniff_steps[e->sniff.step].prompt);
    state_set_status(e->state, status);
}

static void send_listen_window_filter(elm_t *e, uint32_t now_ms) {
    char command[16];
    snprintf(command, sizeof command, "ATCF%03X", e->windows % ELM_LISTEN_WINDOWS * 0x100);
    send_command(e, command, ELM_TIMEOUT_MS, now_ms);
    char status[64];
    snprintf(status, sizeof status, "LISTEN %d/%d\nhands off\n%d%%", e->sniff.step + 1, SNIFF_STEPS,
             100 * e->windows / (ELM_LISTEN_WINDOWS * ELM_LISTEN_ROUNDS));
    state_set_status(e->state, status);
}

bool elm_listen_click(elm_t *e, uint32_t now_ms) {
    if (e->phase != ELM_LISTEN || !e->listen_awaiting_click) {
        return false;
    }
    e->listen_awaiting_click = false;
    e->windows = 0;
    char line[48];
    snprintf(line, sizeof line, "sniff: step %d/%d", e->sniff.step + 1, SNIFF_STEPS);
    e->cb.log(e->cb.ctx, line);
    send_listen_window_filter(e, now_ms);
    return true;
}

void handle_can_listen_answer(elm_t *e, uint32_t now_ms) {
    if (strcmp(e->command, "ATCAF0") == 0) {
        send_command(e, "ATCM700", ELM_TIMEOUT_MS, now_ms);
        return;
    }
    if (strcmp(e->command, "ATCM700") == 0) {
        prompt_listen_step(e);
        return;
    }
    if (strncmp(e->command, "ATCF", 4) == 0) {
        e->monitoring = true;
        e->stop_sent = false;
        e->monitor_ms = now_ms;
        e->monitor_dwell_ms = ELM_LISTEN_DWELL_MS;
        send_command(e, "ATMA", ELM_LISTEN_DWELL_MS + ELM_TIMEOUT_MS, now_ms);
        return;
    }
    // A monitor ended: next window, the next step, or done.
    e->monitoring = false;
    if (++e->windows < ELM_LISTEN_WINDOWS * ELM_LISTEN_ROUNDS) {
        send_listen_window_filter(e, now_ms);
        return;
    }
    if (++e->sniff.step < SNIFF_STEPS) {
        prompt_listen_step(e);
        return;
    }
    sniff_log_candidates(&e->sniff, e->cb.log, e->cb.ctx);
    state_set_status(e->state, "SYNC\nlistening done, see the log");
    elm_start_session(e, e->state, e->cb, now_ms);
}
