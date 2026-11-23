#include "source.h"

#include <stdarg.h>
#include <stdio.h>

#include "ble/ble.h"
#include "hardware/mcp2515.h"
#include "hardware/store.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"

// The watchdog scratch register boot_reason reads (the SDK uses 4 to 7).
// 0 held a reboot that connected to the BLE dongle right away.
#define SCRATCH_CRASH 1
#define HARD_FAULT 0xbadf0017

boot_t boot_reason(void) {
    boot_t why = {
        .froze = watchdog_enable_caused_reboot(),
        .hard_fault = watchdog_hw->scratch[SCRATCH_CRASH] == HARD_FAULT,
    };
    watchdog_hw->scratch[SCRATCH_CRASH] = 0;
    return why;
}

static state_t *source_state;

void source_start(state_t *state) {
    source_state = state;
    if (!SIM) {
        log_line("boot: data source CAN (MCP2515)");
        mcp2515_source_init(state);
    }
}

void poll_data_source(uint32_t now) {
    ble_poll_radio(now);
    if (!SIM && !ble_sending()) {
        mcp2515_poll_bus(now);
    }
}

bool give_click_to_prompt(void) {
    if (ble_handle_click()) {
        return true;
    }
    // The MCP2515's Retry reruns its self test from a fresh boot.
    if (!SIM && mcp2515_wants_retry()) {
        reboot_board();
    }
    return false;
}

void run_settings_action(action_t action) {
    datalog_t *log = store_get_datalog();
    if (action == ACTION_SEND_LOG && log) {
        datalog_flush(log);
        ble_send_log(source_state, log);
    }
}

void __attribute__((noreturn)) reboot_board(void) {
    watchdog_reboot(0, 0, 0);
    for (;;) {
        tight_loop_contents();
    }
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
    reboot_board();
}
