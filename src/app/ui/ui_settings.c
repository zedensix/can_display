#include <stdio.h>

#include "ui_internal.h"

#define SETTINGS_MARGIN 8
#define SETTINGS_HEADER 30
#define SETTINGS_GROUP_HEIGHT 14
#define SETTINGS_ROW_HEIGHT 22
#define SETTINGS_BACK_HEIGHT 22

// The settings page's groups, each a run of rows under a title; Back is
// a button under them. Everything fits: no scrolling.
static const struct {
    const char *title;
    int first, last;
} settings_groups[] = {
    {"DISPLAY", ROW_BRIGHTNESS, ROW_FLIP},
    {"CONNECTION", ROW_SOURCE, ROW_AUTO_CONNECT},
    {"TOOLS", ROW_SEND_LOG, ROW_DISCONNECT},
};

static const char *const settings_labels[SETTINGS_ROWS] = {
    [ROW_BRIGHTNESS] = "Brightness", [ROW_SHIFT_LIGHT] = "Shift light",   [ROW_FLIP] = "Flip screen",
    [ROW_SOURCE] = "Source",         [ROW_AUTO_CONNECT] = "Auto-connect", [ROW_SEND_LOG] = "Send log",
    [ROW_SCAN] = "Scan ECU RAM",     [ROW_LISTEN] = "Listen CAN",         [ROW_DISCONNECT] = "Disconnect",
};

// draw_toggle draws a switch ending at right, centered on cy: lit, its
// knob to the right.
static void draw_toggle(canvas_t *c, const theme_t *t, int right, int cy, bool on) {
    int w = 26, h = 12, x = right - w, y = cy - h / 2;
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    if (on) {
        gfx_fill_rect(c, x, y, w, h, t->accent);
        gfx_fill_rect(c, x + w - 10, y + 2, 8, h - 4, t->background);
    } else {
        draw_outline(c, x, y, w, h, dim);
        gfx_fill_rect(c, x + 2, y + 2, 8, h - 4, dim);
    }
}

// draw_choice draws options side by side ending at right, the chosen one
// filled, centered on cy.
static void draw_choice(canvas_t *c, const theme_t *t, int right, int cy, const char *const options[], int count,
                        int chosen) {
    int w = 0, h = 16;
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    for (int i = 0; i < count; i++) {
        int text = gfx_text_width(label_font, options[i]) + 10;
        w = text > w ? text : w;
    }
    for (int i = 0; i < count; i++) {
        int x = right - (count - i) * w, y = cy - h / 2;
        if (i == chosen) {
            gfx_fill_rect(c, x, y, w, h, t->accent);
        } else {
            draw_outline(c, x, y, w, h, dim);
        }
        draw_text_centered(c, label_font, x + w / 2, y + 12, options[i], i == chosen ? t->background : dim, 1);
    }
}

// draw_settings_value draws row's value or control, ending at right,
// centered on cy.
static void draw_settings_value(canvas_t *c, const theme_t *t, int row, int right, int cy, const settings_t *cfg,
                                bool selected, bool editing) {
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    color_t value = editing ? t->warning : t->accent;
    switch (row) {
    case ROW_BRIGHTNESS: {
        // The gauges' segments: one per brightness step.
        enum { COUNT = (BRIGHTNESS_MAX - BRIGHTNESS_MIN) / BRIGHTNESS_STEP + 1 };
        color_t colors[COUNT];
        for (int i = 0; i < COUNT; i++) {
            colors[i] =
                BRIGHTNESS_MIN + i * BRIGHTNESS_STEP <= cfg->brightness ? value : gfx_blend(t->background, value, 0.2f);
        }
        draw_segment_bar(c, right - 70, cy - 5, 70, 10, COUNT, colors);
        break;
    }
    case ROW_SHIFT_LIGHT: {
        char text[16];
        snprintf(text, sizeof text, editing ? "< %d >" : "%d", cfg->shift_rpm);
        if (!cfg->shift_rpm) {
            snprintf(text, sizeof text, editing ? "< OFF >" : "OFF");
        }
        color_t color = editing ? t->warning : cfg->shift_rpm ? t->accent : dim;
        gfx_text(c, label_font, right - gfx_text_width(label_font, text), cy + 5, text, color, 1);
        break;
    }
    case ROW_FLIP:
        draw_toggle(c, t, right, cy, cfg->flip);
        break;
    case ROW_AUTO_CONNECT:
        draw_toggle(c, t, right, cy, cfg->auto_connect);
        break;
    case ROW_SOURCE: {
        static const char *const sources[] = {"BLE", "CAN"};
        draw_choice(c, t, right, cy, sources, 2, cfg->source == SOURCE_BLE ? 0 : 1);
        break;
    }
    default: // an action
        gfx_text(c, label_font, right - gfx_text_width(label_font, ">"), cy + 5, ">", selected ? t->accent : dim, 1);
        break;
    }
}

void draw_settings_page(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg) {
    int left = SETTINGS_MARGIN, right = c->width - SETTINGS_MARGIN, width = right - left;
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    gfx_text(c, &font_bold12, left + 2, 20, "SETTINGS", t->accent, 1);
    gfx_fill_rect(c, left, 25, width, 2, t->accent);

    int y = SETTINGS_HEADER;
    for (size_t g = 0; g < sizeof settings_groups / sizeof settings_groups[0]; g++) {
        gfx_text(c, label_font, left + 2, y + 11, settings_groups[g].title, gfx_blend(t->background, t->accent, 0.6f),
                 1);
        y += SETTINGS_GROUP_HEIGHT;
        for (int row = settings_groups[g].first; row <= settings_groups[g].last; row++, y += SETTINGS_ROW_HEIGHT) {
            bool selected = row == screen->cursor;
            // With the CAN source, the rows needing the dongle are dimmed and say why.
            bool ble_only = cfg->source != SOURCE_BLE && screen_row_needs_ble(row);
            int h = SETTINGS_ROW_HEIGHT - 2, cy = y + h / 2;
            if (selected) {
                gfx_fill_rect(c, left, y, width, h, gfx_blend(t->background, t->accent, 0.2f));
                gfx_fill_rect(c, left, y, 3, h, t->accent);
            }
            gfx_text(c, label_font, left + 12, cy + 5, settings_labels[row], ble_only ? dim : t->label, 1);
            if (ble_only) {
                const char *why = "BLE";
                gfx_text(c, label_font, right - 12 - gfx_text_width(label_font, why), cy + 5, why, dim, 1);
            } else {
                draw_settings_value(c, t, row, right - 12, cy, cfg, selected, selected && screen->editing);
            }
        }
        y += 2;
    }

    int back_y = c->height - SETTINGS_BACK_HEIGHT - 20;
    if (screen->cursor == ROW_BACK) {
        gfx_fill_rect(c, left, back_y, width, SETTINGS_BACK_HEIGHT, t->accent);
    } else {
        draw_outline(c, left, back_y, width, SETTINGS_BACK_HEIGHT, gfx_blend(t->background, t->accent, 0.5f));
    }
    draw_text_centered(c, label_font, c->width / 2, back_y + 15, "BACK",
                       screen->cursor == ROW_BACK ? t->background : t->accent, 1);
    draw_text_centered(c, label_font, c->width / 2, c->height - 4, "hold: reboot", dim, 1);
}
