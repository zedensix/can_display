#include "screen.h"

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

void screen_turn(screen_t *s, int detents, int views, settings_t *cfg) {
    if (detents == 0) {
        return;
    }
    if (s->page == PAGE_GAUGES) {
        int count = views + 1;
        s->view = ((s->view + detents) % count + count) % count;
    } else if (s->editing) {
        cfg->brightness = clamp(cfg->brightness + detents * BRIGHTNESS_STEP, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
    } else {
        s->cursor = clamp(s->cursor + detents, 0, SETTINGS_ROWS - 1);
    }
}

void screen_press(screen_t *s, settings_t *cfg) {
    if (s->page == PAGE_GAUGES) {
        s->page = PAGE_SETTINGS;
        s->cursor = ROW_BRIGHTNESS;
        s->editing = false;
        return;
    }
    switch (s->cursor) {
    case ROW_BRIGHTNESS:
        s->editing = !s->editing;
        break;
    case ROW_AUTO_CONNECT:
        cfg->auto_connect = !cfg->auto_connect;
        break;
    case ROW_FLIP:
        cfg->flip = !cfg->flip;
        break;
    case ROW_ETHANOL_SENSOR:
        cfg->ethanol_sensor = !cfg->ethanol_sensor;
        break;
    case ROW_BACK:
        s->page = PAGE_GAUGES;
        break;
    }
}
