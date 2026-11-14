// The Raspberry Pi Pico W firmware: reads the GT86's data through an
// ELM327 Bluetooth LE OBD dongle (ble.c) or straight off the CAN bus
// through an MCP2515 (mcp2515.c), reads the flex-fuel sensor on the
// RP2040's ADC, and draws src/app's UI on the ST7789 panel. The `sim`
// build (SIM=1) shows fake data instead, to check the panel without a
// car.

#include <stdarg.h>
#include <stdio.h>

#include "ble.h"
#include "board.h"
#include "display.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"
#include "knob.h"
#include "mcp2515.h"
#include "pico/stdlib.h"
#include "sensors.h"
#include "simdata.h"
#include "store.h"
#include "ui.h"

#ifndef SIM
#define SIM 0
#endif

// The watchdog turns any freeze into a reboot: every pass of the main
// loop feeds it, and nothing may block it longer than this.
#define WATCHDOG_MS 8000
// 20 frames a second. Drawing takes a few ms and the DMA push ~20ms, the
// rest of the frame runs the data source.
#define FRAME_MS 50

// Watchdog scratch registers survive a reboot (not a power cut): they
// tell the next boot why this one ended. The SDK uses 4 to 7.
#define SCRATCH_CONNECT_NOW 0
#define SCRATCH_CRASH 1
#define CONNECT_NOW 0x86c0ffee
#define HARD_FAULT 0xbadf0017

// The whole frame in RAM, 153.6KB of the RP2040's 264KB: the panel's
// controller has no framebuffer the MCU can draw into cheaply, and a
// pixel-per-transaction redraw visibly sweeps the screen.
static color_t frame[PANEL_WIDTH * PANEL_HEIGHT];
static state_t state;

static uint32_t now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

static void source_poll(uint32_t now) {
#if SIM
    (void)now;
#elif DATA_SOURCE == SOURCE_MCP2515
    mcp2515_source_poll(now);
#else
    ble_poll(now);
#endif
}

static bool standby(void) {
#if !SIM && DATA_SOURCE == SOURCE_ELM327_BLE
    return ble_standby();
#else
    return false;
#endif
}

static bool prompt_click(uint32_t now) {
#if !SIM && DATA_SOURCE == SOURCE_ELM327_BLE
    return ble_prompt_click(now);
#else
    (void)now;
    return false;
#endif
}

static void scan_ram(void) {
#if !SIM && DATA_SOURCE == SOURCE_ELM327_BLE
    ble_scan_ram();
#endif
}

static void go_offline(void) {
#if !SIM && DATA_SOURCE == SOURCE_ELM327_BLE
    ble_go_offline();
#endif
}

static void listen_can(void) {
#if !SIM && DATA_SOURCE == SOURCE_ELM327_BLE
    ble_listen_can();
#endif
}

static void __attribute__((noreturn)) reboot(bool connect) {
#if !SIM && DATA_SOURCE == SOURCE_ELM327_BLE
    ble_disconnect();
    for (uint32_t until = now_ms() + 300; now_ms() < until;) {
        source_poll(now_ms());
    }
#endif
    watchdog_hw->scratch[SCRATCH_CONNECT_NOW] = connect ? CONNECT_NOW : 0;
    watchdog_reboot(0, 0, 0);
    for (;;) {
        tight_loop_contents();
    }
}

void reboot_to_connect(void) {
    reboot(true);
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
    reboot(false);
}

// draw renders a frame and starts pushing it, then keeps the data source
// running until it's on the panel. boot_elapsed is how far into the boot
// animation we are, or -1 for a normal frame. It returns false once the
// boot animation is over.
static bool draw(int32_t boot_elapsed, const screen_t *screen, const settings_t *cfg) {
    canvas_t canvas = {frame, PANEL_WIDTH, PANEL_HEIGHT, 0, PANEL_HEIGHT};
    display_finish();
    uint32_t now = now_ms();
    if (boot_elapsed >= 0) {
        if (!ui_render_boot(&canvas, (uint32_t)boot_elapsed, now)) {
            return false;
        }
    } else {
        ui_render(&canvas, &state, screen, cfg, now);
    }
    display_push(&canvas);
    while (display_busy()) {
        source_poll(now_ms());
    }
    return true;
}

#if !SIM
// read_ethanol averages a burst of samples: the RP2040's ADC is noisy.
static void read_ethanol(uint32_t now) {
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += adc_read();
    }
    float at_pin = sensors_raw_to_volts((uint16_t)(sum / 16), ADC_VREF);
    state_set(&state, SIG_ETHANOL, sensors_ethanol_percent(divider_undo(ETHANOL_DIVIDER, at_pin)), now);
}
#endif

int main(void) {
    stdio_init_all();
    bool froze = watchdog_enable_caused_reboot();
    bool connect_now = watchdog_hw->scratch[SCRATCH_CONNECT_NOW] == CONNECT_NOW;
    bool hard_fault = watchdog_hw->scratch[SCRATCH_CRASH] == HARD_FAULT;
    watchdog_hw->scratch[SCRATCH_CONNECT_NOW] = watchdog_hw->scratch[SCRATCH_CRASH] = 0;

    store_init();
    log_line(hard_fault ? "boot: the previous run hit a hard fault"
             : froze    ? "boot: the watchdog reset the board, the previous run froze"
                        : "boot");
    watchdog_enable(WATCHDOG_MS, true);

    settings_t cfg = store_load_settings(), saved = cfg, applied = cfg;
    display_init();
    display_set_flip(cfg.flip);
    display_set_brightness(cfg.brightness);
    knob_init();
    adc_init();
    adc_gpio_init(ETHANOL_PIN);
    adc_select_input(ETHANOL_ADC_INPUT);

#if SIM
    (void)connect_now;
#elif DATA_SOURCE == SOURCE_MCP2515
    (void)connect_now;
    mcp2515_source_init(&state);
#else
    ble_init(&state, cfg.auto_connect || connect_now);
#endif

    screen_t screen = {0};
    for (uint32_t boot = now_ms(); draw((int32_t)(now_ms() - boot), &screen, &cfg);) {
        watchdog_update();
    }

    bool asleep = false;
    for (;;) {
        uint32_t start = now_ms();
        watchdog_update();
        store_serve_log();
#if SIM
        simdata_tick(&state, start);
#else
        if (cfg.ethanol_sensor) {
            read_ethanol(start);
        }
#endif

        bool press, long_press;
        screen_turn(&screen, knob_poll(start, &press, &long_press), ui_tile_count, &cfg);
        // On the gauges (and in standby, where nothing is shown), a waiting
        // connection prompt takes the click.
        if (press && !((screen.page == PAGE_GAUGES || standby()) && prompt_click(start))) {
            screen_press(&screen, &cfg);
        }
        if (screen.scan_requested) {
            screen.scan_requested = false;
            scan_ram();
        }
        if (screen.listen_requested) {
            screen.listen_requested = false;
            listen_can();
        }
        if (screen.disconnect_requested) {
            screen.disconnect_requested = false;
            go_offline();
        }
        if (long_press) {
            log_line("knob: reboot");
            reboot(false);
        }
        if (!settings_equal(cfg, applied)) {
            display_finish();
            display_set_flip(cfg.flip);
            display_set_brightness(cfg.brightness);
            applied = cfg;
        }
        if (screen.page == PAGE_GAUGES && !settings_equal(cfg, saved)) {
            store_save_settings(cfg);
            saved = cfg;
        }

        // In standby, only a click matters: no drawing, no busy loop.
        if (standby()) {
            if (!asleep) {
                display_sleep();
                asleep = true;
            }
            source_poll(now_ms());
            sleep_ms(FRAME_MS);
            continue;
        }
        draw(-1, &screen, &cfg);
        while (now_ms() - start < FRAME_MS && !knob_pending()) {
            source_poll(now_ms());
        }
    }
}
