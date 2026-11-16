#include "ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

// The GT86's own cluster: black, lit red-orange, with white labels. The
// readings take their band's color: green fine, amber warns, red danger,
// which blinks its whole tile.
const theme_t theme_dark = {
    .background = RGB(0x00, 0x00, 0x00),
    .frame = RGB(0x4a, 0x12, 0x08),
    .accent = RGB(0xff, 0x5a, 0x1e),
    .label = RGB(0xf0, 0xf0, 0xf0),
    .good = RGB(0x3c, 0xe0, 0x5a),
    .warning = RGB(0xff, 0xb4, 0x1e),
    .danger = RGB(0xff, 0x20, 0x10),
    .alarm = RGB(0xb8, 0x10, 0x08),
    .alarm_text = RGB(0xff, 0xf0, 0xe0),
    // The one cold color in the cluster, like the GT86's own cold engine
    // lamp: don't rev it yet.
    .cold = RGB(0x2e, 0x8c, 0xff),
    .stale = RGB(0x3a, 0x10, 0x08),
};

// Scale, warning and danger bands per gauge, then the cold band (see
// range_t). Cold is no alarm: oil and coolant start there every morning.
// Oil warms up slower than the coolant, and it's what matters before
// revving hard.
// The tune raised the rev limiter to 7600 (stock: 7450): red from there,
// amber 500 before.
#define RPM_RANGE {0, 8000, -1, 7100, -1, 7600, -1}
// Intake air: ambient plus engine bay heat; past 45 the engine loses
// power, past 60 it's heat-soaked.
#define IAT_RANGE {-10, 80, -1, 45, -1, 60, -1}
#define COOLANT_RANGE {40, 130, -1, 105, -1, 115, 70}
#define OIL_RANGE {40, 150, -1, 110, -1, 120, 80}
#define BATTERY_RANGE {10, 16, 12, 15, 11.5f, 15.5f, -1}
#define AFR_RANGE {10, 18, 12, 16, 11, 17, -1}
#define ETHANOL_RANGE {0, 100, -1, 101, -1, 101, -1}

// Most signals update several times a second. The oil temperature and
// the ethanol % come every 3s through the dongle (ELM_ENGINE_INTERVAL_MS).
#define FAST_STALE_MS 2000
#define SLOW_STALE_MS 10000

// From OIL_CRITICAL, the oil temperature takes the whole screen, on every
// page: the tile alone, red from OIL_RANGE's 120, is easy to miss, and
// 120 comes quickly on a track (the FA20 has no oil cooler).
#define OIL_CRITICAL 130

// No RPM tile: the car's own tachometer is right there. RPM still drives
// the shift light.
const tile_t ui_tiles[] = {
    {"OIL TEMP", "%.0f", DEGREE "C", SIG_OIL, OIL_RANGE, SLOW_STALE_MS},
    {"COOLANT", "%.0f", DEGREE "C", SIG_COOLANT, COOLANT_RANGE, FAST_STALE_MS},
    {"AFR", "%.1f", "", SIG_AFR, AFR_RANGE, FAST_STALE_MS},
    {"INTAKE", "%.0f", DEGREE "C", SIG_IAT, IAT_RANGE, FAST_STALE_MS},
    {"BATTERY", "%.1f", "V", SIG_BATTERY, BATTERY_RANGE, FAST_STALE_MS},
    {"ETHANOL", "%.0f", "%", SIG_ETHANOL, ETHANOL_RANGE, SLOW_STALE_MS},
};
const int ui_tile_count = sizeof ui_tiles / sizeof ui_tiles[0];
const int ui_view_count = sizeof ui_tiles / sizeof ui_tiles[0];

extern const unsigned char logo_bin[];

#define GRID_GAP 3
#define GRID_COLUMNS 2

static const font_t *const label_font = &font_bold9;
// Tried largest first: wider readings ("-14.2V") drop a size or two to
// stay inside their tile.
static const font_t *const value_fonts[] = {&font_bold24, &font_bold18, &font_bold12};

#define PI 3.14159265f

static color_t band_color(const range_t *r, float v, const theme_t *t) {
    if (v < r->cold) {
        return t->cold;
    }
    if (v <= r->danger_low || v >= r->danger_high) {
        return t->danger;
    }
    if (v <= r->warn_low || v >= r->warn_high) {
        return t->warning;
    }
    return t->good;
}

static float scale_fraction(const range_t *r, float v) {
    if (r->max <= r->min) {
        return 0;
    }
    float f = (v - r->min) / (r->max - r->min);
    return f < 0 ? 0 : f > 1 ? 1 : f;
}

// format_tile_value is what tile shows for reading: its text, color and gauge
// fraction, or "--" once stale (fraction -1: nothing lit). Off the
// throttle the AFR is no mixture to judge (the fuel is cut while
// decelerating, it reads lean): no band color, no alarm, white.
static void format_tile_value(const tile_t *tile, const state_t *s, uint32_t now_ms, const theme_t *t, char text[24],
                              color_t *color, float *fraction) {
    reading_t reading = s->readings[tile->signal];
    if (reading_is_stale(&reading, now_ms, tile->stale_ms)) {
        snprintf(text, 24, "--");
        *color = t->stale;
        *fraction = -1;
        return;
    }
    char number[16];
    snprintf(number, sizeof number, tile->format, (double)reading.value);
    snprintf(text, 24, "%s%s", number, tile->unit);
    bool unjudged = tile->signal == SIG_AFR && state_off_throttle(s, now_ms, FAST_STALE_MS);
    *color = unjudged ? t->label : band_color(&tile->range, reading.value, t);
    *fraction = scale_fraction(&tile->range, reading.value);
}

#define MAX_SEGMENTS 40

// gauge_segment_colors colors count gauge segments like a GT86 cluster: lit up
// to fraction, each in the band of the value it stands for, the unlit
// ones a glimmer of that band so the red zone shows at rest. On an alarm
// tile, lit segments take the alarm text color. peak (a fraction, or -1
// for none) marks the session's highest value in the alarm text color.
static void gauge_segment_colors(const range_t *r, float fraction, float peak, int count, bool alarm, const theme_t *t,
                                 color_t out[]) {
    color_t back = alarm ? t->alarm : t->background;
    int lit = fraction < 0 ? 0 : (int)(fraction * (float)count + 0.5f);
    if (fraction > 0 && lit == 0) {
        lit = 1;
    }
    for (int i = 0; i < count; i++) {
        color_t band = band_color(r, r->min + ((float)i + 0.5f) / (float)count * (r->max - r->min), t);
        if (i < lit) {
            out[i] = alarm ? t->alarm_text : band;
        } else {
            out[i] = gfx_blend(back, band, alarm ? 0.35f : 0.2f);
        }
    }
    if (peak >= 0) {
        int at = (int)(peak * (float)count);
        out[at < count ? at : count - 1] = t->alarm_text;
    }
}

// draw_segment_bar draws count segments in a row.
static void draw_segment_bar(canvas_t *c, int x, int y, int w, int h, int count, const color_t colors[]) {
    const int gap = 2;
    int seg = (w + gap) / count;
    x += (w - (seg * count - gap)) / 2;
    for (int i = 0; i < count; i++) {
        gfx_fill_rect(c, x + i * seg, y, seg - gap, h, colors[i]);
    }
}

// draw_segment_arc draws count segments around a ring centered on cx, cy, from
// start degrees over sweep degrees (clockwise, 0 pointing right), out of
// 3px squares: the panel has no line drawing.
static void draw_segment_arc(canvas_t *c, int cx, int cy, int r_in, int r_out, float start, float sweep, int count,
                             const color_t colors[]) {
    float seg = sweep / (float)count;
    float step = 90.0f / (PI * (float)r_out); // about 1.5px at the rim
    for (int i = 0; i < count; i++) {
        float a0 = start + (float)i * seg;
        for (float a = a0; a <= a0 + seg * 0.75f; a += step) {
            float cs = cosf(a * PI / 180), sn = sinf(a * PI / 180);
            for (int r = r_in; r <= r_out; r += 2) {
                gfx_fill_rect(c, cx + (int)lroundf((float)r * cs) - 1, cy + (int)lroundf((float)r * sn) - 1, 3, 3,
                              colors[i]);
            }
        }
    }
}

static void draw_outline(canvas_t *c, int x, int y, int w, int h, color_t color) {
    gfx_fill_rect(c, x, y, w, 1, color);
    gfx_fill_rect(c, x, y + h - 1, w, 1, color);
    gfx_fill_rect(c, x, y, 1, h, color);
    gfx_fill_rect(c, x + w - 1, y, 1, h, color);
}

static void draw_text_centered(canvas_t *c, const font_t *font, int cx, int y, const char *s, color_t color,
                               int scale) {
    gfx_text(c, font, cx - gfx_text_width(font, s) * scale / 2, y, s, color, scale);
}

// A reading in danger blinks: its tile lit red, then not, twice a second.
#define ALARM_BLINK_MS 250

static bool alarm_blink_lit(uint32_t now_ms) {
    return now_ms / ALARM_BLINK_MS % 2 == 0;
}

// draw_text_fitting centers s in the largest font it fits max_width in.
static void draw_text_fitting(canvas_t *c, int cx, int y, int max_width, const char *s, color_t color) {
    static const font_t *const fonts[] = {&font_bold24, &font_bold18, &font_bold12, &font_bold9};
    int i = 0;
    while (i < 3 && gfx_text_width(fonts[i], s) > max_width) {
        i++;
    }
    draw_text_centered(c, fonts[i], cx, y, s, color, 1);
}

#define BAR_SEGMENTS 12

static void draw_tile(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h, const state_t *s,
                      uint32_t now_ms) {
    char text[24];
    color_t color;
    float fraction;
    format_tile_value(tile, s, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_blink_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    draw_outline(c, x, y, w, h, color == t->danger ? t->danger : t->frame);
    int label_y = y + 17;
    gfx_text(c, label_font, x + 8, label_y, tile->label, alarm ? t->alarm_text : t->label, 1);

    const font_t *font = value_fonts[2];
    for (int i = 0; i < 3; i++) {
        if (gfx_text_width(value_fonts[i], text) <= w - 16) {
            font = value_fonts[i];
            break;
        }
    }
    gfx_text(c, font, x + 8, label_y + 42, text, alarm ? t->alarm_text : color, 1);
    color_t colors[BAR_SEGMENTS];
    gauge_segment_colors(&tile->range, fraction, -1, BAR_SEGMENTS, alarm, t, colors);
    draw_segment_bar(c, x + 8, y + h - 16, w - 16, 8, BAR_SEGMENTS, colors);
}

#define DIAL_SEGMENTS 30
#define DIAL_START 135
#define DIAL_SWEEP 270
#define DIAL_WIDTH 14

// draw_tile_dial draws a tile alone, as a round dial like the GT86's
// tachometer: segments around the value, blown up as much as fits
// inside, up to 3 times (chunky, but readable from the seat, without a
// bigger font taking flash).
static void draw_tile_dial(canvas_t *c, const theme_t *t, const tile_t *tile, int x, int y, int w, int h,
                           const state_t *s, uint32_t now_ms) {
    reading_t r = s->readings[tile->signal];
    char text[24];
    color_t color;
    float fraction;
    format_tile_value(tile, s, now_ms, t, text, &color, &fraction);
    bool alarm = color == t->danger && alarm_blink_lit(now_ms);
    if (alarm) {
        gfx_fill_rect(c, x, y, w, h, t->alarm);
    }
    int cx = x + w / 2, r_out = w / 2 - 4, cy = y + r_out + 8, r_in = r_out - DIAL_WIDTH;
    color_t colors[DIAL_SEGMENTS];
    float peak = r.valid ? scale_fraction(&tile->range, r.max) : -1;
    gauge_segment_colors(&tile->range, fraction, peak, DIAL_SEGMENTS, alarm, t, colors);
    draw_segment_arc(c, cx, cy, r_in, r_out, DIAL_START, DIAL_SWEEP, DIAL_SEGMENTS, colors);

    int width = gfx_text_width(&font_bold24, text);
    int scale = 3;
    while (scale > 1 && width * scale > 2 * r_in - 24) {
        scale--;
    }
    // The 24pt digits rise about 26px above their baseline.
    const int cap_height = 26;
    draw_text_centered(c, &font_bold24, cx, cy + cap_height * scale / 2, text, alarm ? t->alarm_text : color, scale);

    // The scale's ends, under the ring's opening, and the label below.
    char end[16];
    int ends_y = cy + r_out * 7 / 10 + 22, ends_dx = r_out * 7 / 10;
    color_t dim = alarm ? t->alarm_text : t->label;
    snprintf(end, sizeof end, tile->format, (double)tile->range.min);
    draw_text_centered(c, label_font, cx - ends_dx, ends_y, end, dim, 1);
    snprintf(end, sizeof end, tile->format, (double)tile->range.max);
    draw_text_centered(c, label_font, cx + ends_dx, ends_y, end, dim, 1);
    // The session's extremes, to read back after a run.
    if (r.valid) {
        char low[16], high[16], line[48];
        snprintf(low, sizeof low, tile->format, (double)r.min);
        snprintf(high, sizeof high, tile->format, (double)r.max);
        snprintf(line, sizeof line, "MIN %s%s   MAX %s%s", low, tile->unit, high, tile->unit);
        draw_text_centered(c, label_font, cx, ends_y + 30, line, alarm ? t->alarm_text : t->label, 1);
    }
    draw_text_centered(c, &font_bold12, cx, y + h - 14, tile->label, dim, 1);
}

// draw_grid draws every tile, from top down.
static void draw_grid(canvas_t *c, const theme_t *t, const state_t *s, int top, uint32_t now_ms) {
    int rows = (ui_tile_count + GRID_COLUMNS - 1) / GRID_COLUMNS;
    int cell_w = c->width / GRID_COLUMNS, cell_h = (c->height - top) / rows;
    for (int i = 0; i < ui_tile_count; i++) {
        int col = i % GRID_COLUMNS, row = i / GRID_COLUMNS;
        draw_tile(c, t, &ui_tiles[i], col * cell_w + GRID_GAP, top + row * cell_h + GRID_GAP, cell_w - 2 * GRID_GAP,
                  cell_h - 2 * GRID_GAP, s, now_ms);
    }
}

int ui_wrap_text(const font_t *font, const char *s, int max_width, char out[][64], int max) {
    int n = 0;
    char line[64] = "";
    while (*s && n < max) {
        while (*s == ' ') {
            s++;
        }
        size_t word_len = strcspn(s, " ");
        if (word_len == 0) {
            break;
        }
        char candidate[64];
        int len = line[0] ? snprintf(candidate, sizeof candidate, "%s %.*s", line, (int)word_len, s)
                          : snprintf(candidate, sizeof candidate, "%.*s", (int)word_len, s);
        // Past the buffer counts as too wide: the word goes on the next line.
        bool too_wide = len >= (int)sizeof candidate || gfx_text_width(font, candidate) > max_width;
        if (line[0] && too_wide) {
            snprintf(out[n++], 64, "%s", line);
            snprintf(line, sizeof line, "%.*s", (int)word_len, s);
        } else {
            snprintf(line, sizeof line, "%s", candidate);
        }
        s += word_len;
    }
    if (n < max && (line[0] || n == 0)) {
        snprintf(out[n++], 64, "%s", line);
    }
    return n;
}

static void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color);

#define STATUS_MARGIN 8
#define STATUS_LINE_HEIGHT 20
#define RING_Y 100
// The spinner ring: the logo's own circle at full size, so the boot
// animation can close it into the logo.
#define RING_IN 73
#define RING_OUT 80
// One turn of the spinner's head, and how far its tail trails (256 to
// the turn).
#define SPIN_MS 1200
#define SPIN_TAIL 110

// ring_px is a quadrant of the ring's pixels, worked out once: offset
// from the center, angle (64 to the quarter turn, clockwise from the
// right) and coverage (255 inside, less on the edges: anti-aliasing).
// The other quadrants are its mirror images.
typedef struct {
    uint8_t dx, dy, angle, alpha;
} ring_px_t;
#define RING_PX_MAX 1400
static ring_px_t ring_px[RING_PX_MAX];
static int ring_px_count;

static void build_spinner_ring(void) {
    for (int dy = 0; dy <= RING_OUT + 1; dy++) {
        for (int dx = 0; dx <= RING_OUT + 1; dx++) {
            float d = sqrtf((float)(dx * dx + dy * dy));
            float cover = fminf(d - ((float)RING_IN - 0.5f), ((float)RING_OUT + 0.5f) - d);
            if (cover <= 0 || ring_px_count == RING_PX_MAX) {
                continue;
            }
            float angle = atan2f((float)dy, (float)dx) / (PI / 2) * 64;
            ring_px[ring_px_count++] =
                (ring_px_t){(uint8_t)dx, (uint8_t)dy, (uint8_t)lroundf(angle), (uint8_t)(fminf(cover, 1) * 255)};
        }
    }
}

// blend_rgb565 blends from into to, t out of 255, in integers: per pixel, floats
// would cost the frame.
static color_t blend_rgb565(color_t from, color_t to, unsigned t) {
    unsigned r = ((from >> 11) * (255 - t) + (to >> 11) * t) / 255;
    unsigned g = (((from >> 5) & 63) * (255 - t) + ((to >> 5) & 63) * t) / 255;
    unsigned b = ((from & 31) * (255 - t) + (to & 31) * t) / 255;
    return (color_t)(r << 11 | g << 5 | b);
}

// draw_spinner draws the ring centered on cx, cy: a lit head going round
// smoothly, a tail fading behind it; done (0 to 1) lights it all.
static void draw_spinner(canvas_t *c, const theme_t *t, int cx, int cy, uint32_t now_ms, float done) {
    if (ring_px_count == 0) {
        build_spinner_ring();
    }
    unsigned head = (unsigned)(now_ms % SPIN_MS * 256 / SPIN_MS);
    unsigned fill = (unsigned)(done * 255);
    static const int8_t sx[4] = {1, -1, -1, 1}, sy[4] = {1, 1, -1, -1};
    for (int i = 0; i < ring_px_count; i++) {
        const ring_px_t *p = &ring_px[i];
        const unsigned angles[4] = {p->angle, 128u - p->angle, 128u + p->angle, 256u - p->angle};
        for (int q = 0; q < 4; q++) {
            unsigned behind = (head - angles[q]) & 255;
            unsigned lit = behind < SPIN_TAIL ? 255 - behind * 255 / SPIN_TAIL : 0;
            lit += (255 - lit) * fill / 255;
            color_t color = blend_rgb565(t->background, blend_rgb565(t->stale, t->accent, lit), p->alpha);
            gfx_fill_rect(c, cx + sx[q] * p->dx, cy + sy[q] * p->dy, 1, 1, color);
        }
    }
}

// draw_status_screen is the source's report while it isn't delivering: the
// logo in a spinner (so a slow step doesn't look frozen), the first
// paragraph as a title, the rest centered and wrapped under it.
static void draw_status_screen(canvas_t *c, const theme_t *t, const char *status, uint32_t now_ms) {
    draw_spinner(c, t, c->width / 2, RING_Y, now_ms, 0);
    draw_logo(c, c->width / 2, RING_Y, 2, t->accent);

    int y = RING_Y + RING_OUT + 34;
    bool first = true;
    char paragraph[192];
    snprintf(paragraph, sizeof paragraph, "%s", status);
    for (char *p = paragraph; p && y < c->height; first = false) {
        char *next = strchr(p, '\n');
        if (next) {
            *next++ = 0;
        }
        const font_t *font = first ? &font_bold12 : label_font;
        char lines[8][64];
        int n = ui_wrap_text(font, p, c->width - 2 * STATUS_MARGIN, lines, 8);
        for (int i = 0; i < n; i++) {
            draw_text_centered(c, font, c->width / 2, y, lines[i], first ? t->accent : t->label, 1);
            y += STATUS_LINE_HEIGHT + (first ? 6 : 0);
        }
        p = next;
    }
}

#define SETTINGS_MARGIN 8
#define SETTINGS_HEADER 34
#define SETTINGS_GROUP_HEIGHT 16
#define SETTINGS_ROW_HEIGHT 20
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

static void draw_settings_page(canvas_t *c, const theme_t *t, const screen_t *screen, const settings_t *cfg) {
    int left = SETTINGS_MARGIN, right = c->width - SETTINGS_MARGIN, width = right - left;
    color_t dim = gfx_blend(t->background, t->label, 0.35f);
    gfx_text(c, &font_bold12, left + 2, 22, "SETTINGS", t->accent, 1);
    gfx_fill_rect(c, left, 29, width, 2, t->accent);

    int y = SETTINGS_HEADER;
    for (size_t g = 0; g < sizeof settings_groups / sizeof settings_groups[0]; g++) {
        gfx_text(c, label_font, left + 2, y + 12, settings_groups[g].title, gfx_blend(t->background, t->accent, 0.6f),
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
            gfx_text(c, label_font, left + 10, cy + 5, settings_labels[row], ble_only ? dim : t->label, 1);
            if (ble_only) {
                const char *why = "BLE";
                gfx_text(c, label_font, right - 8 - gfx_text_width(label_font, why), cy + 5, why, dim, 1);
            } else {
                draw_settings_value(c, t, row, right - 8, cy, cfg, selected, selected && screen->editing);
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

// The shift light is a race car steering wheel's LED bar across the top:
// SHIFT_LEDS / 2 pairs light from both ends towards the middle over the
// last SHIFT_SPAN_RPM before shift_rpm, green, then amber, then red; from
// shift_rpm on, they all flash blue. Narrow LEDs, close together.
#define SHIFT_BAR_HEIGHT 20
#define SHIFT_LEDS 20
#define SHIFT_SPAN_RPM 1500
#define SHIFT_BLINK_MS 60

static void draw_shift_light(canvas_t *c, const theme_t *t, const reading_t *rpm, int shift_rpm, uint32_t now_ms) {
    const int pairs = SHIFT_LEDS / 2, gap = 2, margin = GRID_GAP;
    int led_w = (c->width - 2 * margin - (SHIFT_LEDS - 1) * gap) / SHIFT_LEDS;
    // What the integer LED width leaves over goes to the outer margins.
    int x0 = (c->width - SHIFT_LEDS * led_w - (SHIFT_LEDS - 1) * gap) / 2;
    bool fresh = !reading_is_stale(rpm, now_ms, FAST_STALE_MS);
    bool shift = fresh && rpm->value >= (float)shift_rpm;
    int lit = 0;
    if (fresh && !shift) {
        float from = (float)(shift_rpm - SHIFT_SPAN_RPM);
        lit = rpm->value < from ? 0 : 1 + (int)((rpm->value - from) / ((float)SHIFT_SPAN_RPM / pairs));
    }
    for (int k = 0; k < pairs; k++) {
        color_t color = k < 4 ? t->good : k < 7 ? t->warning : t->danger;
        if (shift) {
            color = now_ms / SHIFT_BLINK_MS % 2 ? t->background : t->cold;
        } else if (k >= lit) {
            color = gfx_blend(t->background, color, 0.15f);
        }
        gfx_fill_rect(c, x0 + k * (led_w + gap), 3, led_w, SHIFT_BAR_HEIGHT - 6, color);
        gfx_fill_rect(c, c->width - x0 - led_w - k * (led_w + gap), 3, led_w, SHIFT_BAR_HEIGHT - 6, color);
    }
}

// draw_oil_overheat_screen is the whole screen for an oil temperature at
// OIL_CRITICAL or more, blinking, in the driver's words: calm down, it's
// getting hot. Lifting off and driving gently cools the oil; shutting the
// engine off would stop it flowing.
static void draw_oil_overheat_screen(canvas_t *c, const theme_t *t, float oil, uint32_t now_ms) {
    bool lit = alarm_blink_lit(now_ms);
    color_t text = lit ? t->alarm_text : t->danger;
    gfx_fill_rect(c, 0, 0, c->width, c->height, lit ? t->alarm : t->background);
    draw_text_centered(c, &font_bold24, c->width / 2, 48, "OIL TEMP", text, 1);

    // Degrees only: with the C it wouldn't fit 3 times as big.
    char value[16];
    snprintf(value, sizeof value, "%.0f" DEGREE, (double)oil);
    int scale = 3;
    while (scale > 1 && gfx_text_width(&font_bold24, value) * scale > c->width - 16) {
        scale--;
    }
    draw_text_centered(c, &font_bold24, c->width / 2, 158 + 13 * scale, value, text, scale);
    // The fonts are ASCII only: no accents.
    draw_text_fitting(c, c->width / 2, 266, c->width - 16, "CALME-TOI", text);
    draw_text_fitting(c, c->width / 2, 304, c->width - 16, "CA CHAUFFE SA MERE", text);
}

static bool oil_is_overheating(const state_t *state, uint32_t now_ms) {
    const reading_t *oil = &state->readings[SIG_OIL];
    return !reading_is_stale(oil, now_ms, SLOW_STALE_MS) && oil->value >= OIL_CRITICAL;
}

bool ui_shows_status_screen(const state_t *state, const screen_t *screen, uint32_t now_ms) {
    return !oil_is_overheating(state, now_ms) && screen->page != PAGE_SETTINGS && state->status[0];
}

void ui_spinner_rows(int panel_height, bool boot, int *top, int *rows) {
    int center = boot ? panel_height / 2 : RING_Y;
    *top = center - RING_OUT - 1;
    *rows = 2 * RING_OUT + 3;
}

void ui_render(canvas_t *c, const state_t *state, const screen_t *screen, const settings_t *cfg, uint32_t now_ms) {
    const theme_t *t = &theme_dark;
    if (oil_is_overheating(state, now_ms)) {
        draw_oil_overheat_screen(c, t, state->readings[SIG_OIL].value, now_ms);
        return;
    }
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    if (screen->page == PAGE_SETTINGS) {
        draw_settings_page(c, t, screen, cfg);
        return;
    }
    if (state->status[0]) {
        draw_status_screen(c, t, state->status, now_ms);
        return;
    }
    // With the shift light on, its bar takes the top of the gauges.
    int top = 0;
    if (cfg->shift_rpm) {
        draw_shift_light(c, t, &state->readings[SIG_RPM], cfg->shift_rpm, now_ms);
        top = SHIFT_BAR_HEIGHT;
    }
    if (screen->view > 0 && screen->view <= ui_tile_count) {
        const tile_t *tile = &ui_tiles[screen->view - 1];
        draw_tile_dial(c, t, tile, GRID_GAP, top + GRID_GAP, c->width - 2 * GRID_GAP, c->height - top - 2 * GRID_GAP,
                       state, now_ms);
    } else {
        draw_grid(c, t, state, top, now_ms);
    }
}

// draw_logo draws the logo centered on cx, cy, shrink times smaller: a
// pixel is lit if any in its shrink x shrink block is.
static void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color) {
    int w = logo_bin[0] << 8 | logo_bin[1], h = logo_bin[2] << 8 | logo_bin[3];
    int stride = (w + 7) / 8;
    int x0 = cx - w / shrink / 2, y0 = cy - h / shrink / 2;
    const unsigned char *rows = logo_bin + 4;
    for (int y = 0; y < h / shrink; y++) {
        for (int x = 0; x < w / shrink; x++) {
            bool on = false;
            for (int k = 0; k < shrink * shrink && !on; k++) {
                int sx = x * shrink + k % shrink, sy = y * shrink + k / shrink;
                on = rows[sy * stride + sx / 8] & (0x80 >> (sx % 8));
            }
            if (on) {
                gfx_fill_rect(c, x0 + x, y0 + y, 1, 1, color);
            }
        }
    }
}

bool ui_render_boot(canvas_t *c, uint32_t elapsed_ms, uint32_t now_ms) {
    if (elapsed_ms >= BOOT_DURATION_MS) {
        return false;
    }
    const theme_t *t = &theme_dark;
    int cx = c->width / 2, cy = c->height / 2;
    gfx_fill_rect(c, 0, 0, c->width, c->height, t->background);
    if (elapsed_ms < BOOT_SPIN_MS) {
        draw_spinner(c, t, cx, cy, now_ms, 0);
    } else if (elapsed_ms < BOOT_SPIN_MS + BOOT_MORPH_MS) {
        float morph = (float)(elapsed_ms - BOOT_SPIN_MS) / BOOT_MORPH_MS;
        draw_logo(c, cx, cy, 1, gfx_blend(t->background, t->accent, morph));
        draw_spinner(c, t, cx, cy, now_ms, morph);
    } else {
        draw_logo(c, cx, cy, 1, t->accent);
    }
    return true;
}
