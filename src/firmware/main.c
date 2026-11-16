// The Raspberry Pi Pico W firmware: reads the GT86's data through an
// ELM327 Bluetooth LE OBD dongle (ble.c) or straight off the CAN bus
// through an MCP2515 (mcp2515.c), and draws src/app's UI on the ST7789
// panel. The `sim`
// build (SIM=1) shows fake data instead, to check the panel without a
// car.

#include <stdio.h>
#include <string.h>

#include "board.h"
#include "data/simdata.h"
#include "hardware/display.h"
#include "hardware/knob.h"
#include "hardware/store.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "source.h"
#include "ui/ui.h"

// The watchdog turns any freeze into a reboot: every pass of the main
// loop feeds it, and nothing may block it longer than this.
#define WATCHDOG_MS 8000
// 20 frames a second. Drawing takes a few ms and the DMA push ~20ms, the
// rest of the frame runs the data source.
#define FRAME_MS 50

// The whole frame in RAM, 153.6KB of the RP2040's 264KB: the panel's
// controller has no framebuffer the MCU can draw into cheaply, and a
// pixel-per-transaction redraw visibly sweeps the screen.
static color_t frame[PANEL_WIDTH * PANEL_HEIGHT];
static state_t state;

// record_datalog_sample samples the gauges into the data log while the engine runs (RPM
// coming in). The sim build's fake data isn't logged.
static void record_datalog_sample(uint32_t now) {
#if !SIM
    static uint32_t last;
    datalog_t *log = store_get_datalog();
    if (log && !reading_is_stale(&state.readings[SIG_RPM], now, 2000) && now - last >= DATALOG_SAMPLE_MS) {
        last = now;
        datalog_sample(log, &state, now);
    }
#else
    (void)now;
#endif
}

// render_and_push_rows renders a frame and starts pushing it, then keeps the data source
// running until it's on the panel. boot_elapsed is how far into the boot
// animation we are, or -1 for a normal frame. It returns false once the
// boot animation is over.
static bool render_and_push_rows(int32_t boot_elapsed, const screen_t *screen, const settings_t *cfg, int top,
                                 int rows) {
    canvas_t canvas = {frame + top * PANEL_WIDTH, PANEL_WIDTH, PANEL_HEIGHT, top, rows};
    display_wait_for_push();
    uint32_t now = now_ms();
    if (boot_elapsed >= 0) {
        if (!ui_render_boot(&canvas, (uint32_t)boot_elapsed, now)) {
            return false;
        }
    } else {
        ui_render(&canvas, &state, screen, cfg, now);
    }
    display_push_rows(&canvas);
    while (display_push_in_progress()) {
        poll_data_source(now_ms());
    }
    return true;
}

int main(void) {
    stdio_init_all();
    boot_t boot = boot_reason();

    store_init();
    log_line(boot.hard_fault ? "boot: the previous run hit a hard fault"
             : boot.froze    ? "boot: the watchdog reset the board, the previous run froze"
                             : "boot");
    watchdog_enable(WATCHDOG_MS, true);

    settings_t cfg = store_load_settings(), saved = cfg, applied = cfg;
    display_init();
    display_set_flip(cfg.flip);
    display_set_brightness(cfg.brightness);
    knob_init();

    source_start(&cfg, &state, boot.connect_now);

    // The boot animation stays within the spinner's rows: after a first
    // whole frame, only those are drawn and pushed.
    screen_t screen = {0};
    int spin_top, spin_rows;
    ui_spinner_rows(PANEL_HEIGHT, true, &spin_top, &spin_rows);
    for (uint32_t boot = now_ms(), first = 1; render_and_push_rows(
             (int32_t)(now_ms() - boot), &screen, &cfg, first ? 0 : spin_top, first ? PANEL_HEIGHT : spin_rows);
         first = 0) {
        watchdog_update();
    }

    // The status shown in the last whole frame, if a status screen, and the choice under it.
    char shown[sizeof state.status] = "";
    choice_t shown_choice = screen.choice;

    bool asleep = false;
    for (;;) {
        uint32_t start = now_ms();
        watchdog_update();
        store_dump_log_on_request();
#if SIM
        simdata_update(&state, start);
#endif

        bool press, long_press;
        int turn = knob_read_input(start, &press, &long_press);
        // On the gauges (and in standby, where nothing is shown), a waiting
        // connection prompt takes the click.
        if (screen_apply_input(&screen, &state, turn, press, ui_view_count, &cfg, start) &&
            !((screen.page == PAGE_GAUGES || in_standby()) && give_click_to_prompt(start))) {
            screen_apply_click(&screen, &cfg);
        }
        remember_connected_dongle(&cfg);
        run_settings_action(screen.action);
        screen.action = ACTION_NONE;
        record_datalog_sample(start);
        if (long_press) {
            log_line("knob: reboot");
            reboot_board(false);
        }
        if (!settings_equal(cfg, applied)) {
            display_wait_for_push();
            display_set_flip(cfg.flip);
            display_set_brightness(cfg.brightness);
            applied = cfg;
        }
        if (screen.page == PAGE_GAUGES && !settings_equal(cfg, saved)) {
            store_save_settings(cfg);
            saved = cfg;
            if (cfg.source != source_running()) {
                log_line("settings: data source changed, rebooting");
                reboot_board(false);
            }
        }

        // In standby, only a click matters: no drawing, no busy loop.
        if (in_standby()) {
            if (!asleep) {
                datalog_t *log = store_get_datalog();
                if (log) {
                    datalog_flush(log);
                }
                display_sleep();
                asleep = true;
            }
            poll_data_source(now_ms());
            sleep_ms(FRAME_MS);
            continue;
        }
        // On an unchanged status screen, only the spinner moves: its rows
        // alone, as often as they can go, for a smooth spin.
        bool status = ui_shows_status_screen(&state, &screen, start);
        if (status && shown[0] && strcmp(shown, state.status) == 0 && shown_choice == screen.choice) {
            ui_spinner_rows(PANEL_HEIGHT, false, &spin_top, &spin_rows);
            render_and_push_rows(-1, &screen, &cfg, spin_top, spin_rows);
            continue;
        }
        render_and_push_rows(-1, &screen, &cfg, 0, PANEL_HEIGHT);
        snprintf(shown, sizeof shown, "%s", status ? state.status : "");
        shown_choice = screen.choice;
        while (now_ms() - start < FRAME_MS && !knob_has_new_input()) {
            poll_data_source(now_ms());
        }
    }
}
