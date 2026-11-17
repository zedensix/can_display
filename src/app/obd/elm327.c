#include <stdio.h>
#include <string.h>

#include "elm_internal.h"

static const char *const init_commands[] = {
    "ATZ",    // reset; answers with its banner, not OK
    "ATE0",   // echo off
    "ATL0",   // no linefeeds after CR
    "ATS0",   // no spaces between hex bytes
    "ATH1",   // print CAN IDs: needed to rebuild frames
    "ATCAF1", // automatic ISO-TP formatting
    // Protocol 6 = ISO 15765-4 CAN, 11bit IDs, 500kbps: what the GT86
    // speaks on its OBD port. Fixing it skips the slow auto-detect. If
    // the dongle refuses to talk to the car, try ATSP0 (auto).
    "ATSP6",
    "ATSH7DF", // requests to the functional address
};
#define INIT_COUNT (int)(sizeof init_commands / sizeof init_commands[0])

static const char *const probe_commands[] = {"ATI",
                                             "ATRV",
                                             "0100",
                                             "0120",
                                             "0140",
                                             "01A0",
                                             "010C",
                                             MULTI_POLL_FAST MULTI_POLL_FRAMES,
                                             MULTI_POLL_SLOW MULTI_POLL_FRAMES};
#define PROBE_COUNT (int)(sizeof probe_commands / sizeof probe_commands[0])

void send_command(elm_t *e, const char *command, uint32_t timeout_ms, uint32_t now_ms) {
    snprintf(e->command, sizeof e->command, "%s", command);
    char line[sizeof e->command + 1];
    snprintf(line, sizeof line, "%s\r", command);
    e->rx_len = 0;
    e->waiting = true;
    e->sent_ms = now_ms;
    e->timeout_ms = timeout_ms;
    e->cb.write(e->cb.ctx, line);
}

void fail_session(elm_t *e, const char *step, const char *error) {
    e->phase = ELM_FAILED;
    e->waiting = false;
    e->cb.failed(e->cb.ctx, step, error);
}

void log_command_answer(elm_t *e, const char *command, char *lines[], int n) {
    char out[160];
    int len = snprintf(out, sizeof out, "elm327: %s ->", command);
    for (int i = 0; i < n && len < (int)sizeof out; i++) {
        len += snprintf(out + len, sizeof out - (size_t)len, "%s%s", i ? " | " : " ", lines[i]);
    }
    e->cb.log(e->cb.ctx, out);
}

static void start_probes(elm_t *e, uint32_t now_ms) {
    e->phase = ELM_PROBE;
    e->step = 0;
    send_command(e, probe_commands[0], ELM_TIMEOUT_MS, now_ms);
}

static void handle_init_answer(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    if (e->step > 0) {
        bool ok = false;
        for (int i = 0; i < n; i++) {
            ok = ok || strcmp(lines[i], "OK") == 0;
        }
        if (!ok) {
            char error[96];
            snprintf(error, sizeof error, "%s answered \"%s\"", e->command, n ? lines[0] : "");
            fail_session(e, "ELM327 init", error);
            return;
        }
    }
    if (++e->step < INIT_COUNT) {
        send_command(e, init_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    e->cb.log(e->cb.ctx, "elm327: ready");
    start_probes(e, now_ms);
}

static void send_next_probe(elm_t *e, uint32_t now_ms) {
    if (++e->step < PROBE_COUNT) {
        send_command(e, probe_commands[e->step], ELM_TIMEOUT_MS, now_ms);
        return;
    }
    start_polling(e, now_ms);
}

static void handle_dongle_answer(elm_t *e, uint32_t now_ms) {
    e->rx[e->rx_len] = 0;
    char *lines[ELM_MAX_LINES];
    int n = elm_split_lines(e->rx, e->command, lines, ELM_MAX_LINES);
    e->attempt = 0;
    switch (e->phase) {
    case ELM_INIT:
        handle_init_answer(e, lines, n, now_ms);
        break;
    case ELM_PROBE:
        log_command_answer(e, e->command, lines, n);
        if (strncmp(e->command, MULTI_POLL_FAST, strlen(MULTI_POLL_FAST)) == 0 ||
            strncmp(e->command, MULTI_POLL_SLOW, strlen(MULTI_POLL_SLOW)) == 0) {
            enable_multi_pid_if_answered(e, lines, n);
        }
        send_next_probe(e, now_ms);
        break;
    case ELM_SCAN:
        handle_ram_scan_answer(e, lines, n, now_ms);
        break;
    case ELM_LISTEN:
        handle_can_listen_answer(e, now_ms);
        break;
    case ELM_POLL:
        handle_poll_answer(e, lines, n, now_ms);
        break;
    default:
        break;
    }
}

void elm_start_session(elm_t *e, state_t *state, elm_callbacks_t cb, uint32_t now_ms) {
    // Not *e = (elm_t){...}: with the sniff table, that temporary would
    // take far more than the Pico's 2KB stack.
    memset(e, 0, sizeof *e);
    e->cb = cb;
    e->state = state;
    e->phase = ELM_INIT;
    e->single_response = true;
    send_command(e, init_commands[0], ELM_RESET_TIMEOUT_MS, now_ms);
}

void elm_feed_dongle_output(elm_t *e, const uint8_t *data, size_t len, uint32_t now_ms) {
    for (size_t i = 0; i < len && e->waiting; i++) {
        // A monitor prints frames until stopped: each line is fed as it
        // comes, the buffer would never hold them all.
        if (e->monitoring && (data[i] == '\r' || data[i] == '\n')) {
            e->rx[e->rx_len] = 0;
            can_frame_t f;
            if (elm_parse_frame(e->rx, &f)) {
                if (e->phase == ELM_LISTEN) {
                    sniff_add_frame(&e->sniff, &f);
                } else if (mode_monitor_frame(e, &f, now_ms) && !e->stop_sent) {
                    // The mode's in: the monitor can stop right away.
                    e->stop_sent = true;
                    e->cb.write(e->cb.ctx, "\r");
                }
            }
            e->rx_len = 0;
        } else if (data[i] == '>') {
            e->waiting = false;
            handle_dongle_answer(e, now_ms);
        } else if (data[i] != 0 && e->rx_len < sizeof e->rx - 1) {
            e->rx[e->rx_len++] = (char)data[i];
        }
    }
}

void elm_check_timers(elm_t *e, uint32_t now_ms) {
    if (e->monitoring && !e->stop_sent && now_ms - e->monitor_ms >= e->monitor_dwell_ms) {
        e->stop_sent = true;
        e->cb.write(e->cb.ctx, "\r");
        return;
    }
    if (!e->waiting || now_ms - e->sent_ms <= e->timeout_ms) {
        return;
    }
    if (e->attempt < ELM_RETRIES) {
        e->attempt++;
        char command[sizeof e->command];
        memcpy(command, e->command, sizeof command);
        send_command(e, command, e->timeout_ms, now_ms);
        return;
    }
    e->attempt = 0;
    char error[64];
    snprintf(error, sizeof error, "no answer to %s", e->command);
    if (e->phase == ELM_PROBE) {
        e->waiting = false;
        e->cb.log(e->cb.ctx, error);
        send_next_probe(e, now_ms);
        return;
    }
    if (e->phase == ELM_LISTEN) {
        e->cb.log(e->cb.ctx, error);
        sniff_log_candidates(&e->sniff, e->cb.log, e->cb.ctx);
        e->state->wait = WAIT_NONE;
        elm_start_session(e, e->state, e->cb, now_ms);
        return;
    }
    if (e->phase == ELM_SCAN) {
        e->waiting = false;
        e->cb.log(e->cb.ctx, error);
        if (strcmp(e->command, "ATSH7DF") == 0) {
            start_polling(e, now_ms);
        } else {
            finish_ram_scan(e, now_ms);
        }
        return;
    }
    fail_session(e, e->phase == ELM_POLL ? "OBD poll" : "ELM327 init", error);
}

void elm_request_ram_scan(elm_t *e) {
    if (e->phase == ELM_POLL) {
        e->scan_requested = true;
    }
}

void elm_request_can_listen(elm_t *e) {
    if (e->phase == ELM_POLL) {
        e->listen_requested = true;
    }
}

bool elm_cancel_ram_scan(elm_t *e) {
    if (e->phase != ELM_SCAN || e->scan_cancelled) {
        return false;
    }
    e->scan_cancelled = true;
    state_set_status(e->state, "DEEP SCAN\ncancelling");
    e->state->wait = WAIT_NONE;
    return true;
}
