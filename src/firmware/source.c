#include "source.h"

#include <stdarg.h>
#include <stdio.h>

#include "ble/ble.h"
#include "hardware/mcp2515.h"
#include "hardware/store.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

// The watchdog scratch registers boot_reason reads. The SDK uses 4 to 7.
#define SCRATCH_CONNECT_NOW 0
#define SCRATCH_CRASH 1
#define CONNECT_NOW 0x86c0ffee
#define HARD_FAULT 0xbadf0017

boot_t boot_reason(void) {
    boot_t why = {
        .froze = watchdog_enable_caused_reboot(),
        .hard_fault = watchdog_hw->scratch[SCRATCH_CRASH] == HARD_FAULT,
        .connect_now = watchdog_hw->scratch[SCRATCH_CONNECT_NOW] == CONNECT_NOW,
    };
    watchdog_hw->scratch[SCRATCH_CONNECT_NOW] = watchdog_hw->scratch[SCRATCH_CRASH] = 0;
    return why;
}

// The data source this boot runs, from the settings: changing it reboots.
static source_t source;

// ble_active is true when the BLE dongle is the source (never in the sim build).
static bool ble_active(void) {
    return !SIM && source == SOURCE_BLE;
}

void poll_data_source(uint32_t now) {
    if (SIM) {
        return;
    }
    if (source == SOURCE_CAN) {
        mcp2515_poll_bus(now);
    } else {
        ble_poll_radio(now);
    }
}

bool in_standby(void) {
    return ble_active() && ble_in_standby();
}

bool give_click_to_prompt(uint32_t now) {
    // The MCP2515's Retry reruns its self test from a fresh boot.
    if (!SIM && source == SOURCE_CAN && mcp2515_wants_retry()) {
        reboot_board(false);
    }
    return ble_active() && ble_handle_click(now);
}

void remember_connected_dongle(settings_t *cfg) {
    dongle_t d;
    if (ble_active() && ble_take_connected_dongle(&d)) {
        cfg->dongle = d;
    }
}

void source_start(const settings_t *cfg, state_t *state, bool connect) {
    source = cfg->source;
    log_line("boot: data source %s", source == SOURCE_CAN ? "CAN (MCP2515)" : "BLE dongle");
    if (SIM) {
        (void)connect;
    } else if (source == SOURCE_CAN) {
        mcp2515_source_init(state);
    } else {
        ble_init(state, cfg->auto_connect || connect, &cfg->dongle);
    }
}

source_t source_running(void) {
    return source;
}

void hold_retries(bool hold) {
    if (ble_active()) {
        ble_hold_retries(hold);
    }
}

void run_settings_action(action_t action) {
    if (!ble_active()) {
        return;
    }
    switch (action) {
    case ACTION_SEND_LOG: {
        datalog_t *log = store_get_datalog();
        if (log) {
            datalog_flush(log);
            ble_send_log(log);
        }
        break;
    }
    case ACTION_SCAN:
        ble_request_ram_scan();
        break;
    case ACTION_DISCONNECT:
        ble_go_offline();
        break;
    case ACTION_NONE:
        break;
    }
}

void __attribute__((noreturn)) reboot_board(bool connect) {
    if (ble_active()) {
        ble_disconnect_before_reboot();
        for (uint32_t until = now_ms() + 300; now_ms() < until;) {
            poll_data_source(now_ms());
        }
    }
    watchdog_hw->scratch[SCRATCH_CONNECT_NOW] = connect ? CONNECT_NOW : 0;
    watchdog_reboot(0, 0, 0);
    for (;;) {
        tight_loop_contents();
    }
}

void reboot_to_connect(void) {
    reboot_board(true);
}

// isr_hardfault replaces the SDK's, which hangs: with nothing safe left to
// do, it leaves a mark for the next boot's log and reboots.
void isr_hardfault(void) {
    watchdog_hw->scratch[SCRATCH_CRASH] = HARD_FAULT;
    watchdog_reboot(0, 0, 0);
    for (;;) {
    }
}

// crash_panic is the SDK's panic (PICO_PANIC_FUNCTION): it logs the
// message to flash, which the serial console alone would lose, and
// reboots.
void __attribute__((noreturn)) crash_panic(const char *fmt, ...) {
    char message[120];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof message, fmt, args);
    va_end(args);
    log_line("panic: %s", message);
    reboot_board(false);
}
