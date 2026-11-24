#include "screen.h"

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

void screen_apply_turn(screen_t *s, int detents, int views, settings_t *cfg) {
    if (detents == 0) {
        return;
    }
    if (s->page == PAGE_GAUGES) {
        int count = views + 1;
        s->view = ((s->view + detents) % count + count) % count;
    } else if (s->editing && s->cursor == ROW_BRIGHTNESS) {
        cfg->brightness = clamp_int(cfg->brightness + detents * BRIGHTNESS_STEP, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    } else if (s->editing && s->cursor == ROW_SHIFT_LIGHT) {
        // Below the minimum is off; turning up from off starts there.
        int rpm = cfg->shift_rpm ? cfg->shift_rpm + detents * SHIFT_RPM_STEP
                  : detents > 0  ? SHIFT_RPM_MIN + (detents - 1) * SHIFT_RPM_STEP
                                 : 0;
        cfg->shift_rpm = rpm < SHIFT_RPM_MIN ? 0 : clamp_int(rpm, SHIFT_RPM_MIN, SHIFT_RPM_MAX);
    } else {
        s->cursor = clamp_int(s->cursor + detents, 0, SETTINGS_ROWS - 1);
    }
}

void screen_apply_click(screen_t *s, settings_t *cfg) {
    if (s->page == PAGE_GAUGES) {
        s->page = PAGE_SETTINGS;
        s->cursor = ROW_BRIGHTNESS;
        s->editing = false;
        return;
    }
    switch (s->cursor) {
    case ROW_BRIGHTNESS:
    case ROW_SHIFT_LIGHT:
        s->editing = !s->editing;
        break;
    case ROW_FLIP:
        cfg->flip = !cfg->flip;
        break;
    case ROW_LANDSCAPE:
        cfg->landscape = !cfg->landscape;
        break;
    case ROW_SEND_LOG:
        s->action = ACTION_SEND_LOG;
        s->page = PAGE_GAUGES;
        break;
    case ROW_BACK:
        s->page = PAGE_GAUGES;
        break;
    }
}

bool screen_shows_doors(const screen_t *s, const state_t *state, uint32_t now) {
    return s->page == PAGE_GAUGES && !state->status[0] && (state_doors(state, now) & ~s->doors_dismissed);
}

bool screen_apply_input(screen_t *s, const state_t *state, int detents, bool press, int views, settings_t *cfg,
                        uint32_t now) {
    s->doors_dismissed &= state_doors(state, now);
    if (detents || press) {
        s->input_ms = now;
        if (s->idle) {
            s->idle = false;
            return false;
        }
    }
    if (press && !s->idle && screen_shows_doors(s, state, now)) {
        s->doors_dismissed = state_doors(state, now);
        return false;
    }
    bool choosing = s->page == PAGE_GAUGES && state->wait != WAIT_NONE;
    if (choosing && detents) {
        s->choice = detents > 0 ? CHOICE_SETTINGS : CHOICE_RETRY;
    } else {
        screen_apply_turn(s, detents, views, cfg);
    }
    if (press && choosing && s->choice == CHOICE_SETTINGS) {
        screen_apply_click(s, cfg);
        return false;
    }
    // Nothing coming in: Retry, or a status with nothing to choose (the
    // MCP2515 waiting for the car to wake the bus).
    bool unconnected = state->status[0] != 0;
    s->idle = s->page == PAGE_GAUGES && unconnected && now - s->input_ms >= SCREEN_IDLE_MS;
    return press;
}
