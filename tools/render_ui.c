// render_ui draws the UI's screens with fixed readings, as the panel
// would show them, into PPM files: one per screen and sheet.ppm with all
// of them in a grid, SHEET_COLUMNS wide. `mise run ui` turns them into ui/*.png.

#include <stdio.h>
#include <string.h>

#include "obd/sniff.h"
#include "ui/ui.h"

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

// state_while_driving is a session so far (the extremes) and its latest readings.
static state_t state_while_driving(void) {
    state_t s = {0};
    const float low[SIG_COUNT] = {[SIG_RPM] = 820,       [SIG_COOLANT] = 84, [SIG_IAT] = 22,     [SIG_OIL] = 88,
                                  [SIG_BATTERY] = 13.9f, [SIG_AFR] = 11.8f,  [SIG_ETHANOL] = 67, [SIG_THROTTLE] = 14};
    const float high[SIG_COUNT] = {[SIG_RPM] = 7350,      [SIG_COOLANT] = 96, [SIG_IAT] = 41,     [SIG_OIL] = 112,
                                   [SIG_BATTERY] = 14.4f, [SIG_AFR] = 15.9f,  [SIG_ETHANOL] = 69, [SIG_THROTTLE] = 92};
    const float v[SIG_COUNT] = {[SIG_RPM] = 6400,      [SIG_COOLANT] = 91, [SIG_IAT] = 31,     [SIG_OIL] = 104,
                                [SIG_BATTERY] = 14.2f, [SIG_AFR] = 14.7f,  [SIG_ETHANOL] = 68, [SIG_THROTTLE] = 45};
    for (int i = 0; i < SIG_COUNT; i++) {
        state_set_reading(&s, (signal_t)i, low[i], now - 60000);
        state_set_reading(&s, (signal_t)i, high[i], now - 60000);
        state_set_reading(&s, (signal_t)i, v[i], now);
    }
    return s;
}

static settings_t cfg = {.brightness = BRIGHTNESS_MAX, .auto_connect = true, .shift_rpm = 7000};

static void render_screen(canvas_t *c, const state_t *s, screen_t screen) {
    ui_render(c, s, &screen, &cfg, now);
}

static void shot_grid(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){0});
}

// The oil too hot, the AFR lean, the intake heat-soaked, the ethanol
// stale; near the shift point, every LED pair but the last lit.
static state_t state_with_alarms(void) {
    state_t s = state_while_driving();
    s.readings[SIG_RPM].value = 6900;
    s.readings[SIG_OIL].value = 124;
    s.readings[SIG_AFR].value = 17.2f;
    s.readings[SIG_IAT].value = 63;
    s.readings[SIG_ETHANOL].updated_ms = 0;
    return s;
}

static void shot_alarms(canvas_t *c) {
    state_t s = state_with_alarms();
    render_screen(c, &s, (screen_t){0});
}

// The same alarms between two blinks.
static void shot_alarms_unlit(canvas_t *c) {
    state_t s = state_with_alarms();
    screen_t screen = {0};
    ui_render(c, &s, &screen, &cfg, now + 250);
}

// Past the shift light's RPM: every LED flashes blue.
static void shot_shift(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_RPM].value = 7150;
    render_screen(c, &s, (screen_t){0});
}

// A cold start: oil and coolant not warm yet.
static void shot_cold(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_COOLANT].value = 52;
    s.readings[SIG_OIL].value = 41;
    s.readings[SIG_IAT].value = 18;
    s.readings[SIG_RPM].value = 1450;
    render_screen(c, &s, (screen_t){0});
}

// Oil past the critical temperature takes the whole screen.
static void shot_oil_critical(canvas_t *c) {
    state_t s = state_while_driving();
    s.readings[SIG_OIL].value = 134;
    render_screen(c, &s, (screen_t){0});
}

static void shot_oil(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 1});
}

static void shot_coolant(canvas_t *c) {
    state_t s = state_while_driving();
    render_screen(c, &s, (screen_t){.view = 2});
}

static void render_status_screen(canvas_t *c, const char *text) {
    state_t s = {0};
    snprintf(s.status, sizeof s.status, "%s", text);
    render_screen(c, &s, (screen_t){0});
}

// A prompt waiting for the driver's choice, the knob on choice.
static void render_choice_screen(canvas_t *c, const char *text, choice_t choice) {
    state_t s = {.wait = WAIT_CHOICE};
    snprintf(s.status, sizeof s.status, "%s", text);
    render_screen(c, &s, (screen_t){.choice = choice});
}

static void shot_ready(canvas_t *c) {
    render_choice_screen(c, "READY\nIOS-Vlink\n(last used)", CHOICE_CONNECT);
}

static void shot_offline(canvas_t *c) {
    render_choice_screen(c, "OFFLINE\nIOS-Vlink", CHOICE_SETTINGS);
}

// The screensaver, on the 86's logo (the parts' logos stay out of git):
// the shine across it, then fading in.
static void shot_showcar(canvas_t *c) {
    state_t s = {.wait = WAIT_CHOICE};
    ui_render(c, &s, &(screen_t){.idle = true}, &cfg, 1300);
}

static void shot_showcar_later(canvas_t *c) {
    state_t s = {.wait = WAIT_CHOICE};
    ui_render(c, &s, &(screen_t){.idle = true}, &cfg, 300);
}

static void shot_connecting(canvas_t *c) {
    render_status_screen(c, "LINKING\n41:42:86:9A:C0:33");
}

static void shot_failed(canvas_t *c) {
    render_status_screen(
        c, "NO LINK\nBLE connect failed:\nservice discovery shot_failed (ATT 0x1f, 0 services)\npress to retry");
}

static void shot_scan(canvas_t *c) {
    render_status_screen(c, "DEEP SCAN\n37 KB of ECU RAM\n3 candidates");
}

// A CAN listen's longest prompt, as elm327.c words it.
static void shot_listen(canvas_t *c) {
    char text[128];
    snprintf(text, sizeof text, "LISTEN 4/%d\n%s\nthen click", SNIFF_STEPS, sniff_steps[3].prompt);
    render_status_screen(c, text);
}

static void shot_settings(canvas_t *c) {
    state_t s = {0};
    render_screen(c, &s, (screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SHIFT_LIGHT, .editing = true});
}

// With the CAN source, the rows needing the dongle dimmed.
static void shot_settings_can(canvas_t *c) {
    state_t s = {0};
    settings_t can = cfg;
    can.source = SOURCE_CAN;
    ui_render(c, &s, &(screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SOURCE}, &can, now);
}

// The boot animation: spinning, then closing into the logo.
static void shot_boot_spin(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS / 2, now);
}

static void shot_boot_morph(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS + BOOT_MORPH_MS / 2, now);
}

static void shot_boot_logo(canvas_t *c) {
    ui_render_boot(c, BOOT_SPIN_MS + BOOT_MORPH_MS + 1, now);
}

static const shot_t shots[] = {
    {"grid", shot_grid},
    {"alarms", shot_alarms},
    {"shot_alarms_unlit", shot_alarms_unlit},
    {"oil_critical", shot_oil_critical},
    {"cold", shot_cold},
    {"shift", shot_shift},
    {"oil", shot_oil},
    {"coolant", shot_coolant},
    {"ready", shot_ready},
    {"offline", shot_offline},
    {"connecting", shot_connecting},
    {"scan", shot_scan},
    {"listen", shot_listen},
    {"failed", shot_failed},
    {"showcar", shot_showcar},
    {"showcar_later", shot_showcar_later},
    {"settings", shot_settings},
    {"settings_can", shot_settings_can},
    {"shot_boot_spin", shot_boot_spin},
    {"shot_boot_morph", shot_boot_morph},
    {"shot_boot_logo", shot_boot_logo},
};
#define SHOTS (int)(sizeof shots / sizeof shots[0])

#define SHEET_ROWS ((SHOTS + SHEET_COLUMNS - 1) / SHEET_COLUMNS)
#define SHEET_W (SHEET_COLUMNS * (W + GAP) - GAP)
#define SHEET_H (SHEET_ROWS * (H + GAP) - GAP)
static unsigned char sheet[SHEET_H][SHEET_W][3];

static void rgb565_to_rgb888(color_t c, unsigned char out[3]) {
    out[0] = (unsigned char)((c >> 11) << 3);
    out[1] = (unsigned char)(((c >> 5) & 0x3f) << 2);
    out[2] = (unsigned char)((c & 0x1f) << 3);
}

static int save_ppm(const char *dir, const char *name, const unsigned char *data, int w, int h) {
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

// The README's showcase: these screens side by side, twice their size
// (each pixel a 2x2 block: crisp, not smoothed), on the log page's dark.
static const char *const showcase_names[] = {"grid", "oil", "alarms", "ready", "settings", "showcar"};
#define SHOWCASE_COUNT (int)(sizeof showcase_names / sizeof showcase_names[0])
#define SHOWCASE_SCALE 2
#define SHOWCASE_GAP 32
#define SHOWCASE_W (SHOWCASE_COUNT * (W * SHOWCASE_SCALE + SHOWCASE_GAP) + SHOWCASE_GAP)
#define SHOWCASE_H (H * SHOWCASE_SCALE + 2 * SHOWCASE_GAP)
static unsigned char showcase[SHOWCASE_H][SHOWCASE_W][3];
static unsigned char shot[SHOTS][H][W][3];

static void compose_showcase(void) {
    for (int y = 0; y < SHOWCASE_H; y++) {
        for (int x = 0; x < SHOWCASE_W; x++) {
            memcpy(showcase[y][x], (unsigned char[3]){13, 5, 3}, 3);
        }
    }
    for (int i = 0; i < SHOWCASE_COUNT; i++) {
        int k = 0;
        while (k < SHOTS && strcmp(shots[k].name, showcase_names[i]) != 0) {
            k++;
        }
        int left = SHOWCASE_GAP + i * (W * SHOWCASE_SCALE + SHOWCASE_GAP);
        for (int y = 0; y < H * SHOWCASE_SCALE && k < SHOTS; y++) {
            for (int x = 0; x < W * SHOWCASE_SCALE; x++) {
                memcpy(showcase[SHOWCASE_GAP + y][left + x], shot[k][y / SHOWCASE_SCALE][x / SHOWCASE_SCALE], 3);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <output dir>\n", argv[0]);
        return 2;
    }
    memset(sheet, 0x40, sizeof sheet);
    for (int k = 0; k < SHOTS; k++) {
        canvas_t c = {pixels, W, H, 0, H};
        shots[k].draw(&c);
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                rgb565_to_rgb888(pixels[y * W + x], shot[k][y][x]);
                memcpy(sheet[k / SHEET_COLUMNS * (H + GAP) + y][k % SHEET_COLUMNS * (W + GAP) + x], shot[k][y][x], 3);
            }
        }
        if (save_ppm(argv[1], shots[k].name, &shot[k][0][0][0], W, H)) {
            return 1;
        }
    }
    compose_showcase();
    return save_ppm(argv[1], "sheet", &sheet[0][0][0], SHEET_W, SHEET_H) ||
           save_ppm(argv[1], "showcase", &showcase[0][0][0], SHOWCASE_W, SHOWCASE_H);
}
