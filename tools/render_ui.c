// render_ui draws the UI's screens with fixed readings, as the panel
// would show them, into PPM files: one per screen and sheet.ppm with all
// of them in a grid, SHEET_COLUMNS wide. `mise run ui` turns them into ui/*.png.

#include <stdio.h>
#include <string.h>

#include "ui.h"

// The panel's size, as in src/firmware/board.h.
#define W 240
#define H 320
#define GAP 8
#define SHEET_COLUMNS 3

static color_t pixels[W * H];

typedef struct {
    const char *name;
    void (*draw)(canvas_t *c);
} shot_t;

static const uint32_t now = 100000;

// driving is a session so far (the extremes) and its latest readings.
static state_t driving(void) {
    state_t s = {0};
    const float low[SIG_COUNT] = {[SIG_RPM] = 820,     [SIG_COOLANT] = 84,   [SIG_IAT] = 22, [SIG_OIL] = 88,
                                  [SIG_BATTERY] = 13.9f, [SIG_AFR] = 11.8f, [SIG_ETHANOL] = 67};
    const float high[SIG_COUNT] = {[SIG_RPM] = 7350,    [SIG_COOLANT] = 96,   [SIG_IAT] = 41, [SIG_OIL] = 112,
                                   [SIG_BATTERY] = 14.4f, [SIG_AFR] = 15.9f, [SIG_ETHANOL] = 69};
    const float v[SIG_COUNT] = {[SIG_RPM] = 6400,    [SIG_COOLANT] = 91,   [SIG_IAT] = 31, [SIG_OIL] = 104,
                                [SIG_BATTERY] = 14.2f, [SIG_AFR] = 14.7f, [SIG_ETHANOL] = 68};
    for (int i = 0; i < SIG_COUNT; i++) {
        state_set(&s, (signal_t)i, low[i], now);
        state_set(&s, (signal_t)i, high[i], now);
        state_set(&s, (signal_t)i, v[i], now);
    }
    return s;
}

static settings_t cfg = {.brightness = BRIGHTNESS_MAX, .auto_connect = true, .shift_rpm = 7000};

static void render(canvas_t *c, const state_t *s, screen_t screen) {
    ui_render(c, s, &screen, &cfg, now);
}

static void grid(canvas_t *c) {
    state_t s = driving();
    render(c, &s, (screen_t){0});
}

// The oil too hot, the AFR lean, the intake heat-soaked, the ethanol
// stale; near the shift point, every LED pair but the last lit.
static state_t alarming(void) {
    state_t s = driving();
    s.readings[SIG_RPM].value = 6900;
    s.readings[SIG_OIL].value = 124;
    s.readings[SIG_AFR].value = 17.2f;
    s.readings[SIG_IAT].value = 63;
    s.readings[SIG_ETHANOL].updated_ms = 0;
    return s;
}

static void alarms(canvas_t *c) {
    state_t s = alarming();
    render(c, &s, (screen_t){0});
}

// The same alarms between two blinks.
static void alarms_unlit(canvas_t *c) {
    state_t s = alarming();
    screen_t screen = {0};
    ui_render(c, &s, &screen, &cfg, now + 250);
}

// Past the shift light's RPM: every LED flashes blue.
static void shift(canvas_t *c) {
    state_t s = driving();
    s.readings[SIG_RPM].value = 7150;
    render(c, &s, (screen_t){0});
}

// A cold start: oil and coolant not warm yet.
static void cold(canvas_t *c) {
    state_t s = driving();
    s.readings[SIG_COOLANT].value = 52;
    s.readings[SIG_OIL].value = 41;
    s.readings[SIG_IAT].value = 18;
    s.readings[SIG_RPM].value = 1450;
    render(c, &s, (screen_t){0});
}

// Oil past the critical temperature takes the whole screen.
static void oil_critical(canvas_t *c) {
    state_t s = driving();
    s.readings[SIG_OIL].value = 134;
    render(c, &s, (screen_t){0});
}

static void oil(canvas_t *c) {
    state_t s = driving();
    render(c, &s, (screen_t){.view = 1});
}

static void coolant(canvas_t *c) {
    state_t s = driving();
    render(c, &s, (screen_t){.view = 2});
}

static void status(canvas_t *c, const char *text) {
    state_t s = {0};
    snprintf(s.status, sizeof s.status, "%s", text);
    render(c, &s, (screen_t){0});
}

static void connecting(canvas_t *c) {
    status(c, "LINKING\n41:42:86:9A:C0:33");
}

static void failed(canvas_t *c) {
    status(c, "NO LINK\nBLE connect failed:\nservice discovery failed (ATT 0x1f, 0 services)\npress to retry");
}

static void scan(canvas_t *c) {
    status(c, "DEEP SCAN\n37 KB of ECU RAM\n3 candidates");
}

static void settings(canvas_t *c) {
    state_t s = {0};
    render(c, &s, (screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SHIFT_LIGHT, .editing = true});
}

// The boot animation: spinning, then closing into the logo.
static void boot_spin(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS / 2, now);
}

static void boot_morph(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS + BOOT_MORPH_MS / 2, now);
}

static void boot_logo(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS + BOOT_MORPH_MS + 1, now);
}

static const shot_t shots[] = {
    {"grid", grid},         {"alarms", alarms}, {"alarms_unlit", alarms_unlit}, {"oil_critical", oil_critical}, {"cold", cold}, {"shift", shift}, {"oil", oil},           {"coolant", coolant},
    {"connecting", connecting}, {"scan", scan}, {"failed", failed}, {"settings", settings}, {"boot_spin", boot_spin}, {"boot_morph", boot_morph}, {"boot_logo", boot_logo},
};
#define SHOTS (int)(sizeof shots / sizeof shots[0])

#define SHEET_ROWS ((SHOTS + SHEET_COLUMNS - 1) / SHEET_COLUMNS)
#define SHEET_W (SHEET_COLUMNS * (W + GAP) - GAP)
#define SHEET_H (SHEET_ROWS * (H + GAP) - GAP)
static unsigned char sheet[SHEET_H][SHEET_W][3];

static void rgb(color_t c, unsigned char out[3]) {
    out[0] = (unsigned char)((c >> 11) << 3);
    out[1] = (unsigned char)(((c >> 5) & 0x3f) << 2);
    out[2] = (unsigned char)((c & 0x1f) << 3);
}

static int save(const char *dir, const char *name, const unsigned char *data, int w, int h) {
    char path[512];
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    FILE *f = fopen(path, "wb");
    if (!f) {
        perror(path);
        return 1;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    fwrite(data, 3, (size_t)(w * h), f);
    return fclose(f) != 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <output dir>\n", argv[0]);
        return 2;
    }
    memset(sheet, 0x40, sizeof sheet);
    static unsigned char shot[H][W][3];
    for (int k = 0; k < SHOTS; k++) {
        canvas_t c = {pixels, W, H, 0, H};
        shots[k].draw(&c);
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                rgb(pixels[y * W + x], shot[y][x]);
                memcpy(sheet[k / SHEET_COLUMNS * (H + GAP) + y][k % SHEET_COLUMNS * (W + GAP) + x], shot[y][x], 3);
            }
        }
        if (save(argv[1], shots[k].name, &shot[0][0][0], W, H)) {
            return 1;
        }
    }
    return save(argv[1], "sheet", &sheet[0][0][0], SHEET_W, SHEET_H);
}
