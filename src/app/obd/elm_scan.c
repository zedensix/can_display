#include <stdio.h>
#include <string.h>

#include "elm_internal.h"
#include "sensors.h"

// The RAM scan (asked from the settings page) looks for where the engine
// ECU keeps the ethanol %, to
// read it there without tapping the flex-fuel sensor's wire. This ECU
// ignores Subaru SSM reads (A0) and locks its ROM behind security access
// (UDS 23 answers 7F 23 33), but reads its RAM, from 0xFFF80000, with a
// UDS ReadMemoryByAddress: 23 14, 4 address bytes, 1 size byte (checked
// 2026-11-14). A float that is a whole number of ECU ADC steps once
// turned back into the sensor's volts (25%/V - 12.5%, the tune's scaling)
// is logged as a candidate, as is one that is such a voltage itself.
static void send_ram_scan_read(elm_t *e, uint32_t now_ms) {
    char command[sizeof e->command];
    snprintf(command, sizeof command, "2314%08lX%02X", (unsigned long)e->scan_addr, ELM_SCAN_CHUNK);
    send_command(e, command, ELM_TIMEOUT_MS, now_ms);
}

static void check_ram_word_for_ethanol(elm_t *e, uint32_t addr, const uint8_t *b) {
    uint32_t bits = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
    float f;
    memcpy(&f, &bits, sizeof f);
    const char *what = NULL;
    if (f >= 1 && f <= 100 && sensors_is_whole_ecu_adc_step((f + 12.5f) / 25)) {
        what = "ethanol %";
    } else if (f >= ETHANOL_SENSOR_MIN_VOLTS && f <= ETHANOL_SENSOR_MAX_VOLTS && sensors_is_whole_ecu_adc_step(f)) {
        what = "sensor volts";
    }
    if (what && e->scan_candidates++ < ELM_SCAN_LOGGED) {
        char line[64];
        snprintf(line, sizeof line, "scan: %08lX = %.3f (%s?)", (unsigned long)addr, (double)f, what);
        e->cb.log(e->cb.ctx, line);
    }
}

void start_ram_scan(elm_t *e, uint32_t now_ms) {
    e->scan_requested = false;
    e->phase = ELM_SCAN;
    e->scan_addr = ELM_SCAN_START;
    e->scan_candidates = 0;
    e->scan_cancelled = false;
    e->cb.log(e->cb.ctx, "scan: started");
    state_set_status(e->state, "DEEP SCAN\nreaching the ECU");
    e->state->wait = WAIT_CANCEL;
    send_command(e, "ATSH7E0", ELM_TIMEOUT_MS, now_ms);
}

void finish_ram_scan(elm_t *e, uint32_t now_ms) {
    char line[80];
    snprintf(line, sizeof line, "scan: stopped at %08lX, %d candidates", (unsigned long)e->scan_addr,
             e->scan_candidates);
    e->cb.log(e->cb.ctx, line);
    e->state->wait = WAIT_NONE;
    send_command(e, "ATSH7DF", ELM_TIMEOUT_MS, now_ms);
}

void handle_ram_scan_answer(elm_t *e, char *lines[], int n, uint32_t now_ms) {
    if (strcmp(e->command, "ATSH7DF") == 0) {
        start_polling(e, now_ms);
        return;
    }
    // Cancelled: the answer in flight is the last one.
    if (e->scan_cancelled) {
        finish_ram_scan(e, now_ms);
        return;
    }
    if (strcmp(e->command, "ATSH7E0") == 0) {
        send_ram_scan_read(e, now_ms);
        return;
    }
    isotp_t answer = {0};
    bool whole = false;
    for (int i = 0; i < n; i++) {
        can_frame_t f;
        if (elm_parse_frame(lines[i], &f) && f.id == OBD_RESPONSE_ECU1 && isotp_add_frame(&answer, &f)) {
            whole = true;
        }
    }
    if (!whole || answer.len != 1 + ELM_SCAN_CHUNK || answer.data[0] != 0x63) {
        log_command_answer(e, e->command, lines, n);
        finish_ram_scan(e, now_ms);
        return;
    }
    for (int i = 0; i < ELM_SCAN_CHUNK; i += 4) {
        check_ram_word_for_ethanol(e, e->scan_addr + (uint32_t)i, answer.data + 1 + i);
    }
    e->scan_addr += ELM_SCAN_CHUNK;
    uint32_t done = e->scan_addr - ELM_SCAN_START;
    if (done >= ELM_SCAN_MAX) {
        finish_ram_scan(e, now_ms);
        return;
    }
    char status[64];
    snprintf(status, sizeof status, "DEEP SCAN\n%lu KB of ECU RAM\n%d candidates", (unsigned long)(done / 1024),
             e->scan_candidates);
    state_set_status(e->state, status);
    send_ram_scan_read(e, now_ms);
}
