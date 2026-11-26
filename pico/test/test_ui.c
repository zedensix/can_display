#include "check.h"
#include "data/signals.h"
#include "obd/sensors.h"
#include "store/settings.h"
#include "ui/screen.h"
#include "ui/ui.h"

void test_screen(void) {
    screen_t s = {0};
    settings_t cfg = SETTINGS_DEFAULT;
    screen_apply_turn(&s, 2, 6, &cfg);
    CHECK(s.view == 2, "view %d after 2 detents, want 2", s.view);
    screen_apply_turn(&s, -3, 6, &cfg);
    CHECK(s.view == 6, "view %d going back past the grid, want the last gauge", s.view);
    screen_apply_turn(&s, 1, 6, &cfg);
    CHECK(s.view == 0, "view %d after the last gauge, want the grid", s.view);

    screen_apply_click(&s, &cfg);
    CHECK(s.page == PAGE_SETTINGS && s.cursor == ROW_BRIGHTNESS, "a click didn't open the settings");
    screen_apply_turn(&s, -1, 6, &cfg);
    CHECK(cfg.brightness == BRIGHTNESS_MAX, "turning without editing changed the brightness");
    screen_apply_click(&s, &cfg);
    screen_apply_turn(&s, -3, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(cfg.brightness == BRIGHTNESS_MAX - 3 * BRIGHTNESS_STEP && !s.editing, "brightness %d, want 3 steps down",
          cfg.brightness);
    screen_apply_click(&s, &cfg);
    screen_apply_turn(&s, -50, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(cfg.brightness == BRIGHTNESS_MIN, "brightness %d, want it stopped at the minimum", cfg.brightness);
    screen_apply_turn(&s, 1, 6, &cfg);
    screen_apply_click(&s, &cfg);
    screen_apply_turn(&s, 1, 6, &cfg);
    CHECK(cfg.shift_rpm == SHIFT_RPM_MIN, "shift light %d turned up from off, want %d", cfg.shift_rpm, SHIFT_RPM_MIN);
    screen_apply_turn(&s, 50, 6, &cfg);
    CHECK(cfg.shift_rpm == SHIFT_RPM_MAX, "shift light %d, want it stopped at %d", cfg.shift_rpm, SHIFT_RPM_MAX);
    screen_apply_turn(&s, -100, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(cfg.shift_rpm == 0, "shift light %d turned all the way down, want off", cfg.shift_rpm);
    screen_apply_turn(&s, 1, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(cfg.flip, "a click on flip didn't turn it on");
    screen_apply_turn(&s, 1, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(s.action == ACTION_SEND_LOG && s.page == PAGE_GAUGES, "a click on Send log gave action %d", s.action);
    s.action = ACTION_NONE;
    screen_apply_click(&s, &cfg);
    screen_apply_turn(&s, 10, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(s.page == PAGE_GAUGES, "a click on Back didn't return to the gauges");
}

void test_sensors(void) {
    CHECK(sensors_is_whole_ecu_adc_step(1234 * ECU_ADC_STEP_VOLTS), "a whole number of ADC steps is off the grid");
    CHECK(!sensors_is_whole_ecu_adc_step(1234.5f * ECU_ADC_STEP_VOLTS), "half an ADC step is on the grid");
}

void test_extremes(void) {
    state_t s = {0};
    state_set_reading(&s, SIG_AFR, 14.7f, 1);
    state_set_reading(&s, SIG_AFR, 11.9f, 2);
    state_set_reading(&s, SIG_AFR, 16.2f, 3);
    state_set_reading(&s, SIG_AFR, 14.1f, 4);
    reading_t r = s.readings[SIG_AFR];
    CHECK(nearly_equal(r.value, 14.1f) && nearly_equal(r.min, 11.9f) && nearly_equal(r.max, 16.2f),
          "afr %f, min %f, max %f", (double)r.value, (double)r.min, (double)r.max);
}

// A lean AFR off the throttle is a fuel cut, not an alarm: closed is
// the session's lowest throttle plus a margin, and stays closed a moment
// after the foot comes back, while the AFR catches up.
void test_off_throttle(void) {
    state_t s = {0};
    CHECK(!state_off_throttle(&s, 0, 2000), "no throttle reading counted as off the throttle");
    state_set_reading(&s, SIG_THROTTLE, 15, 0); // idling
    CHECK(state_off_throttle(&s, 0, 2000), "idling isn't off the throttle");
    state_set_reading(&s, SIG_THROTTLE, 60, 5000);
    CHECK(state_off_throttle(&s, 5000, 2000) == false, "60%% counted as off the throttle 5s after idling");
    state_set_reading(&s, SIG_THROTTLE, 13, 6000); // lifting off, below the idle opening
    CHECK(state_off_throttle(&s, 6000, 2000), "lifting off isn't off the throttle");
    state_set_reading(&s, SIG_THROTTLE, 40, 6500);
    CHECK(state_off_throttle(&s, 6500, 2000), "back on the throttle 0.5s ago, the AFR's lag isn't covered");
    state_set_reading(&s, SIG_THROTTLE, 40, 6000 + THROTTLE_REOPEN_HOLD_MS + 1);
    CHECK(!state_off_throttle(&s, 6000 + THROTTLE_REOPEN_HOLD_MS + 1, 2000), "still off the throttle after the hold");
    CHECK(!state_off_throttle(&s, 20000, 2000), "a stale throttle counted as off the throttle");
    state_set_reading(&s, SIG_THROTTLE, 13 + THROTTLE_CLOSED_MARGIN, 30000);
    CHECK(state_off_throttle(&s, 30000, 2000), "the margin above the closed throttle isn't closed");
}

void test_wrap(void) {
    const char *text = "BLE connect failed: no known ELM327 serial service on this device";
    char lines[8][64];
    int n = ui_wrap_text(&font_text, 0, text, 216, lines, 8);
    CHECK(n >= 2, "wrap gave %d line(s), want the long error split", n);
    char joined[256] = "";
    for (int i = 0; i < n; i++) {
        CHECK(gfx_text_width(&font_text, lines[i], 0) <= 216, "line %d is too wide", i);
        strcat(joined, i ? " " : "");
        strcat(joined, lines[i]);
    }
    CHECK(strcmp(joined, text) == 0, "rejoined lines = %s", joined);
}

// The MCP2515's self test failed: turning picks Retry or Settings, a
// click on Settings opens them, on Retry it's left for the data source.
// Left alone SCREEN_IDLE_MS, the screensaver comes on; the next input
// only wakes it.
void test_choice_and_idle(void) {
    settings_t cfg = SETTINGS_DEFAULT;
    state_t state = {.wait = WAIT_RETRY, .status = "CAN SELF TEST"};
    screen_t s = {0};
    CHECK(screen_apply_input(&s, &state, 0, true, 6, &cfg, 10) && s.page == PAGE_GAUGES,
          "a click on Retry wasn't left for the data source");
    screen_apply_input(&s, &state, 1, false, 6, &cfg, 20);
    CHECK(s.choice == CHOICE_SETTINGS && s.view == 0, "turning picked %d, view %d: want Settings, no view change",
          s.choice, s.view);
    CHECK(!screen_apply_input(&s, &state, 0, true, 6, &cfg, 30) && s.page == PAGE_SETTINGS,
          "a click on Settings didn't open them");
    s.page = PAGE_GAUGES;
    screen_apply_input(&s, &state, -1, false, 6, &cfg, 40);
    CHECK(s.choice == CHOICE_RETRY, "turning back didn't pick Retry");

    screen_apply_input(&s, &state, 0, false, 6, &cfg, 40 + SCREEN_IDLE_MS - 1);
    CHECK(!s.idle, "the screensaver came on before SCREEN_IDLE_MS");
    screen_apply_input(&s, &state, 0, false, 6, &cfg, 40 + SCREEN_IDLE_MS);
    CHECK(s.idle, "no screensaver after SCREEN_IDLE_MS left alone");
    CHECK(!screen_apply_input(&s, &state, 0, true, 6, &cfg, 50000) && !s.idle && s.page == PAGE_GAUGES,
          "waking the screensaver did more than wake it");

    state.wait = WAIT_NONE;
    state_set_status(&state, "");
    screen_apply_input(&s, &state, 0, false, 6, &cfg, 200000);
    CHECK(!s.idle, "the screensaver came on with the data source delivering");
    screen_apply_input(&s, &state, 1, false, 6, &cfg, 200001);
    CHECK(s.view == 1, "turning with nothing to choose didn't change the view");

    // The MCP2515 waiting for the car: a status, nothing to choose.
    state_set_status(&state, "WAITING FOR THE CAR");
    screen_apply_input(&s, &state, 0, false, 6, &cfg, 200001 + SCREEN_IDLE_MS);
    CHECK(s.idle, "no screensaver while the CAN bus sleeps");
}

// Near the rev limiter the whole page flashes red; under it, it doesn't.
void test_limiter_flash(void) {
    static color_t pixels[240 * 320];
    canvas_t c = {pixels, 320, 240, 0, 240};
    state_t s = {0};
    screen_t screen = {0};
    settings_t cfg = SETTINGS_DEFAULT;
    color_t red = theme_dark.alarm;
    // 200ms falls in the flash's lit half, 300ms in the dark one.
    state_set_reading(&s, SIG_RPM, 7450, 200);
    ui_render(&c, &s, &screen, &cfg, 200);
    CHECK(pixels[0] == red && pixels[320 * 240 - 1] == red, "7450 rpm, lit half: the screen isn't red");
    ui_render(&c, &s, &screen, &cfg, 300);
    CHECK(pixels[320 * 240 - 1] != red, "7450 rpm, dark half: the screen stays red");
    state_set_reading(&s, SIG_RPM, 7000, 400);
    ui_render(&c, &s, &screen, &cfg, 400);
    CHECK(pixels[320 * 240 - 1] != red, "7000 rpm: the screen flashed");
}

// Every page draws without touching a pixel outside the canvas (the
// sanitizers watch the buffer's ends), and the gauges keep the info strip
// at the bottom.
void test_pages(void) {
    static color_t pixels[320 * 240];
    state_t s = {0};
    for (int i = 0; i < SIG_COUNT; i++) {
        state_set_reading(&s, (signal_t)i, 50, 1000);
    }
    settings_t cfg = SETTINGS_DEFAULT;
    int w = 320, h = 240;
    canvas_t c = {pixels, w, h, 0, h};
    for (int view = 0; view <= ui_view_count; view++) {
        ui_render(&c, &s, &(screen_t){.view = view}, &cfg, 1000);
    }
    ui_render(&c, &s, &(screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SEND_LOG}, &cfg, 1000);
    state_t waiting = {.wait = WAIT_RETRY, .status = "CAN SELF TEST\nsomething long enough to wrap over a few lines"};
    ui_render(&c, &waiting, &(screen_t){0}, &cfg, 1000);
    // A door open takes the page.
    state_t open = s;
    state_set_doors(&open, DOOR_DRIVER | DOOR_PASSENGER | DOOR_BOOT, 1000);
    ui_render(&c, &open, &(screen_t){0}, &cfg, 1000);
    CHECK(pixels[(h - 26) * w + w / 2] != theme_dark.frame, "%dx%d: the strip under the door screen", w, h);
    // The strip's separator, a line across the bottom of the gauges.
    ui_render(&c, &s, &(screen_t){0}, &cfg, 1000);
    CHECK(pixels[(h - 26) * w + w / 2] == theme_dark.frame, "%dx%d: no info strip at the bottom", w, h);
}

// Doors read count until DOORS_STALE_MS; never read, none.
void test_doors(void) {
    state_t s = {0};
    CHECK(state_doors(&s, 0) == 0, "doors open without a reading");
    state_set_doors(&s, DOOR_BOOT, 0);
    CHECK(state_doors(&s, DOORS_STALE_MS) == DOOR_BOOT, "the boot read at 0 not open");
    CHECK(state_doors(&s, DOORS_STALE_MS + 1) == 0, "a stale reading still counts");

    // Dismissed: a click puts the door screen away, another door brings it
    // back, and a door shut then opened again does too.
    settings_t cfg = SETTINGS_DEFAULT;
    screen_t screen = {0};
    state_set_doors(&s, DOOR_DRIVER, 100);
    CHECK(screen_shows_doors(&screen, &s, 100), "the driver's door open, no door screen");
    CHECK(!screen_apply_input(&screen, &s, 0, true, ui_view_count, &cfg, 100), "the dismiss click left to the caller");
    CHECK(!screen_shows_doors(&screen, &s, 100) && screen.page == PAGE_GAUGES, "dismissed, the door screen stays");
    state_set_doors(&s, DOOR_DRIVER | DOOR_BOOT, 200);
    CHECK(screen_shows_doors(&screen, &s, 200), "the boot opened after a dismiss, no door screen");
    screen_apply_input(&screen, &s, 0, true, ui_view_count, &cfg, 200);
    state_set_doors(&s, 0, 300);
    screen_apply_input(&screen, &s, 0, false, ui_view_count, &cfg, 300);
    state_set_doors(&s, DOOR_DRIVER, 400);
    screen_apply_input(&screen, &s, 0, false, ui_view_count, &cfg, 400);
    CHECK(screen_shows_doors(&screen, &s, 400), "the driver's door shut and opened again, no door screen");
}
