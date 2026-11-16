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
    CHECK(cfg.source == SOURCE_CAN, "a click on the data source didn't switch it to CAN");
    // With the CAN source, the rows needing the dongle do nothing.
    for (int row = 0; row < SETTINGS_ROWS; row++) {
        if (screen_row_needs_ble(row)) {
            screen_t at = {.page = PAGE_SETTINGS, .cursor = row};
            settings_t before = cfg;
            screen_apply_click(&at, &cfg);
            CHECK(at.page == PAGE_SETTINGS && at.action == ACTION_NONE && settings_equal(cfg, before),
                  "row %d acted with the CAN source", row);
        }
    }
    screen_apply_click(&s, &cfg);
    CHECK(cfg.source == SOURCE_BLE, "a second click didn't switch the data source back to BLE");
    screen_apply_turn(&s, 1, 6, &cfg);
    screen_apply_click(&s, &cfg);
    CHECK(cfg.auto_connect, "a click on auto-connect didn't turn it on");
    static const struct {
        int row;
        action_t action;
    } actions[] = {{ROW_SEND_LOG, ACTION_SEND_LOG},
                   {ROW_SCAN, ACTION_SCAN},
                   {ROW_LISTEN, ACTION_LISTEN},
                   {ROW_DISCONNECT, ACTION_DISCONNECT}};
    for (size_t i = 0; i < sizeof actions / sizeof actions[0]; i++) {
        screen_apply_turn(&s, actions[i].row - s.cursor, 6, &cfg);
        screen_apply_click(&s, &cfg);
        CHECK(s.action == actions[i].action && s.page == PAGE_GAUGES, "a click on row %d gave action %d, want %d",
              actions[i].row, s.action, actions[i].action);
        s.action = ACTION_NONE;
        screen_apply_click(&s, &cfg);
    }
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
    int n = ui_wrap_text(&font_bold9, text, 216, lines, 8);
    CHECK(n >= 2, "wrap gave %d line(s), want the long error split", n);
    char joined[256] = "";
    for (int i = 0; i < n; i++) {
        CHECK(gfx_text_width(&font_bold9, lines[i]) <= 216, "line %d is too wide", i);
        strcat(joined, i ? " " : "");
        strcat(joined, lines[i]);
    }
    CHECK(strcmp(joined, text) == 0, "rejoined lines = %s", joined);
}

// Nothing connected, READY waits for a choice: turning picks Connect or
// Settings, a click on Settings opens them, on Connect it's left for the
// data source. Left alone SCREEN_IDLE_MS, the screensaver comes on; the
// next input only wakes it.
void test_choice_and_idle(void) {
    settings_t cfg = SETTINGS_DEFAULT;
    state_t state = {.wait = WAIT_CHOICE};
    screen_t s = {0};
    CHECK(screen_apply_input(&s, &state, 0, true, 6, &cfg, 10) && s.page == PAGE_GAUGES,
          "a click on Connect wasn't left for the data source");
    screen_apply_input(&s, &state, 1, false, 6, &cfg, 20);
    CHECK(s.choice == CHOICE_SETTINGS && s.view == 0, "turning picked %d, view %d: want Settings, no view change",
          s.choice, s.view);
    CHECK(!screen_apply_input(&s, &state, 0, true, 6, &cfg, 30) && s.page == PAGE_SETTINGS,
          "a click on Settings didn't open them");
    s.page = PAGE_GAUGES;
    screen_apply_input(&s, &state, -1, false, 6, &cfg, 40);
    CHECK(s.choice == CHOICE_CONNECT, "turning back didn't pick Connect");

    screen_apply_input(&s, &state, 0, false, 6, &cfg, 40 + SCREEN_IDLE_MS - 1);
    CHECK(!s.idle, "the screensaver came on before SCREEN_IDLE_MS");
    screen_apply_input(&s, &state, 0, false, 6, &cfg, 40 + SCREEN_IDLE_MS);
    CHECK(s.idle, "no screensaver after SCREEN_IDLE_MS left alone");
    CHECK(!screen_apply_input(&s, &state, 0, true, 6, &cfg, 50000) && !s.idle && s.page == PAGE_GAUGES,
          "waking the screensaver did more than wake it");

    state.wait = WAIT_NONE;
    screen_apply_input(&s, &state, 0, false, 6, &cfg, 200000);
    CHECK(!s.idle, "the screensaver came on with the data source delivering");
    screen_apply_input(&s, &state, 1, false, 6, &cfg, 200001);
    CHECK(s.view == 1, "turning with nothing to choose didn't change the view");
}
