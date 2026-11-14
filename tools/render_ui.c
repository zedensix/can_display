// render_ui draws the UI's screens with fixed readings, as the panel
// would show them, into PPM files: one per screen and sheet.ppm with all
// of them side by side. `mise run ui` turns them into ui/*.png.

#include <stdio.h>
#include <string.h>

#include "ui.h"

// The panel's size, as in src/firmware/board.h.
#define W 240
#define H 320
#define GAP 8

static color_t pixels[W * H];

typedef struct {
    const char *name;
    void (*draw)(canvas_t *c);
} shot_t;

static const uint32_t now = 100000;

static state_t driving(void) {
    state_t s = {0};
    const float v[SIG_COUNT] = {[SIG_RPM] = 6850,    [SIG_COOLANT] = 91, [SIG_OIL] = 104,
                                [SIG_BATTERY] = 14.2f, [SIG_AFR] = 14.7f, [SIG_ETHANOL] = 68};
    for (int i = 0; i < SIG_COUNT; i++) {
        s.readings[i] = (reading_t){v[i], true, now};
    }
    return s;
}

static void render(canvas_t *c, const state_t *s, screen_t screen) {
    settings_t cfg = SETTINGS_DEFAULT;
    ui_render(c, s, &screen, &cfg, now);
}

static void grid(canvas_t *c) {
    state_t s = driving();
    render(c, &s, (screen_t){0});
}

// Past the redline, the oil too hot, the AFR lean, the ethanol stale.
static void alarms(canvas_t *c) {
    state_t s = driving();
    s.readings[SIG_RPM].value = 7450;
    s.readings[SIG_OIL].value = 124;
    s.readings[SIG_AFR].value = 17.2f;
    s.readings[SIG_ETHANOL].updated_ms = 0;
    render(c, &s, (screen_t){0});
}

static void rpm(canvas_t *c) {
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
    render(c, &s, (screen_t){.page = PAGE_SETTINGS, .cursor = ROW_AUTO_CONNECT});
}

static void boot(canvas_t *c) {
    ui_render_boot(c, BOOT_SPLASH_MS / 2, now);
}

static const shot_t shots[] = {
    {"grid", grid},         {"alarms", alarms}, {"rpm", rpm},           {"coolant", coolant},
    {"connecting", connecting}, {"scan", scan}, {"failed", failed}, {"settings", settings}, {"boot", boot},
};
#define SHOTS (int)(sizeof shots / sizeof shots[0])

static unsigned char sheet[H][SHOTS * (W + GAP) - GAP][3];

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
                memcpy(sheet[y][k * (W + GAP) + x], shot[y][x], 3);
            }
        }
        if (save(argv[1], shots[k].name, &shot[0][0][0], W, H)) {
            return 1;
        }
    }
    return save(argv[1], "sheet", &sheet[0][0][0], SHOTS * (W + GAP) - GAP, H);
}
