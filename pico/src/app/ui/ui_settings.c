// The settings page, in the Android app's settings' look: the title in
// the accent, the groups' titles dim and letter-spaced, the rows white,
// the one under the cursor a pill.

#include <stdio.h>

#include "ui_internal.h"

#define SETTINGS_MARGIN 8
#define SETTINGS_HEADER 36
#define SETTINGS_GROUP_HEIGHT 20
#define SETTINGS_ROW_HEIGHT 28
#define SETTINGS_BACK_HEIGHT 28

// The settings page's groups, each a run of rows under a title; Back is
// a button under them. Everything fits: no scrolling.
static const struct {
    const char *title;
    int first, last;
} settings_groups[] = {
    {"DISPLAY", ROW_BRIGHTNESS, ROW_FLIP},
    {"TOOLS", ROW_SEND_LOG, ROW_SEND_LOG},
};

static const char *const settings_labels[SETTINGS_ROWS] = {
    [ROW_BRIGHTNESS] = "Brightness",
    [ROW_SHIFT_LIGHT] = "Shift light",
    [ROW_FLIP] = "Flip screen",
    [ROW_SEND_LOG] = "Send log",
};

// draw_toggle draws a switch ending at right, centered on cy, on bg: a
// pill, its round knob to the right when on.
static void draw_toggle(canvas_t *c, const theme_t *t, int right, int cy, bool on, color_t bg) {
    int w = 28, h = 14, x = right - w, y = cy - h / 2, knob = h - 4;
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    if (on) {
        fill_round_rect(c, x, y, w, h, h / 2, t->accent, bg);
        fill_round_rect(c, x + w - 2 - knob, y + 2, knob, knob, knob / 2, t->background, t->accent);
    } else {
        outline_round_rect(c, x, y, w, h, h / 2, dim, bg);
        fill_round_rect(c, x + 2, y + 2, knob, knob, knob / 2, dim, bg);
    }
}

// draw_settings_value draws row's value or control, ending at right,
// centered on cy.
static void draw_settings_value(canvas_t *c, const theme_t *t, int row, int right, int cy, const settings_t *cfg,
                                bool selected, bool editing, color_t bg) {
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
        draw_segment_bar(c, right - 70, cy - 5, 70, 10, COUNT, 2, colors);
        break;
    }
    case ROW_SHIFT_LIGHT: {
        char text[16];
        snprintf(text, sizeof text, editing ? "< %d >" : "%d", cfg->shift_rpm);
        if (!cfg->shift_rpm) {
            snprintf(text, sizeof text, editing ? "< OFF >" : "OFF");
        }
        color_t color = editing ? t->warning : cfg->shift_rpm ? t->accent : dim;
        draw_text_right(c, &font_text, right, cy + font_text.cap / 2, text, color, 0);
        break;
    }
    case ROW_FLIP:
        draw_toggle(c, t, right, cy, cfg->flip, bg);
        break;
    default: // an action
        draw_text_right(c, &font_text, right, cy + font_text.cap / 2, ">", selected ? t->accent : dim, 0);
        break;
    }
}

// draw_settings_page draws the groups top down, Back under them. When they
// don't fit above Back (240px high), they scroll to keep the
// row under the cursor in the middle; whatever wouldn't show whole is
// left out.
void draw_settings_page(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg) {
    int left = SETTINGS_MARGIN, right = c->width - SETTINGS_MARGIN, width = right - left;
    int back_y = c->height - SETTINGS_BACK_HEIGHT - 20, view_h = back_y - 2 - SETTINGS_HEADER;
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    gfx_text(c, &font_value, left + 2, 28, "SETTINGS", t->accent, 3);

    // Where the groups reach, and the cursor's row, from the top of the list.
    int content_h = 0, cursor_y = 0;
    for (size_t g = 0; g < sizeof settings_groups / sizeof settings_groups[0]; g++) {
        content_h += SETTINGS_GROUP_HEIGHT;
        for (int row = settings_groups[g].first; row <= settings_groups[g].last; row++) {
            cursor_y = row == screen->cursor ? content_h : cursor_y;
            content_h += SETTINGS_ROW_HEIGHT;
        }
        content_h += 2;
    }
    int scroll = 0;
    if (content_h > view_h) {
        scroll = screen->cursor == ROW_BACK ? content_h - view_h : cursor_y + SETTINGS_ROW_HEIGHT / 2 - view_h / 2;
        scroll = scroll < 0 ? 0 : scroll > content_h - view_h ? content_h - view_h : scroll;
    }
    int top = SETTINGS_HEADER, bottom = SETTINGS_HEADER + view_h;

    int y = SETTINGS_HEADER - scroll;
    for (size_t g = 0; g < sizeof settings_groups / sizeof settings_groups[0]; g++) {
        if (y >= top && y + SETTINGS_GROUP_HEIGHT <= bottom) {
            gfx_text(c, &font_label, left + 2, y + 16, settings_groups[g].title, t->dim, 2);
        }
        y += SETTINGS_GROUP_HEIGHT;
        for (int row = settings_groups[g].first; row <= settings_groups[g].last; row++, y += SETTINGS_ROW_HEIGHT) {
            if (y < top || y + SETTINGS_ROW_HEIGHT > bottom) {
                continue;
            }
            bool selected = row == screen->cursor;
            int h = SETTINGS_ROW_HEIGHT - 2, cy = y + h / 2;
            // The row under the cursor: a pill, tinted, its edge lit.
            color_t bg = selected ? gfx_blend(t->background, t->accent, 0.18f) : t->background;
            if (selected) {
                fill_round_rect(c, left, y, width, h, h / 2, bg, t->background);
                outline_round_rect(c, left, y, width, h, h / 2, gfx_blend(t->background, t->accent, 0.7f),
                                   t->background);
            }
            gfx_text(c, &font_text, left + 12, cy + font_text.cap / 2, settings_labels[row], t->label, 0);
            draw_settings_value(c, t, row, right - 12, cy, cfg, selected, selected && screen->editing, bg);
        }
        y += 2;
    }

    draw_button(c, t, left, back_y, width, SETTINGS_BACK_HEIGHT, ICON_BACK, "Back", screen->cursor == ROW_BACK);
    draw_text_centered(c, &font_small, c->width / 2, c->height - 5, "hold: reboot", dim, 1);
}
