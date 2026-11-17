// render_ui draws the UI's screens with fixed readings, as the panel
// would show them, into PPM files: one per screen and sheet.ppm with all
// of them in a grid, SHEET_COLUMNS wide. `mise run ui` turns them into
// ui/*.png. The screens themselves are in render_ui_shots.c (the gauges)
// and render_ui_screens.c (the rest).

#include <stdio.h>
#include <string.h>

#include "render_ui.h"

#define GAP 8
#define SHEET_COLUMNS 3
// MAX_SHOTS sizes the buffers: main refuses more.
#define MAX_SHOTS 64

static color_t pixels[W * H];
// Every shot, the gauges' then the others'.
static shot_t shots[MAX_SHOTS];
static int shot_count;

#define SHEET_ROWS ((MAX_SHOTS + SHEET_COLUMNS - 1) / SHEET_COLUMNS)
// The sheet's cells are H square, for both orientations.
#define SHEET_W (SHEET_COLUMNS * (H + GAP) - GAP)
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
// Each shot's pixels, rows of its own width.
static unsigned char shot[MAX_SHOTS][W * H][3];

static void compose_showcase(void) {
    for (int y = 0; y < SHOWCASE_H; y++) {
        for (int x = 0; x < SHOWCASE_W; x++) {
            memcpy(showcase[y][x], (unsigned char[3]){13, 5, 3}, 3);
        }
    }
    for (int i = 0; i < SHOWCASE_COUNT; i++) {
        int k = 0;
        while (k < shot_count && strcmp(shots[k].name, showcase_names[i]) != 0) {
            k++;
        }
        int left = SHOWCASE_GAP + i * (W * SHOWCASE_SCALE + SHOWCASE_GAP);
        for (int y = 0; y < H * SHOWCASE_SCALE && k < shot_count; y++) {
            for (int x = 0; x < W * SHOWCASE_SCALE; x++) {
                memcpy(showcase[SHOWCASE_GAP + y][left + x], shot[k][y / SHOWCASE_SCALE * W + x / SHOWCASE_SCALE], 3);
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <output dir>\n", argv[0]);
        return 2;
    }
    if (gauge_shot_count + screen_shot_count > MAX_SHOTS) {
        fprintf(stderr, "%d screens, MAX_SHOTS is %d\n", gauge_shot_count + screen_shot_count, MAX_SHOTS);
        return 1;
    }
    for (int k = 0; k < gauge_shot_count; k++) {
        shots[shot_count++] = gauge_shots[k];
    }
    for (int k = 0; k < screen_shot_count; k++) {
        shots[shot_count++] = screen_shots[k];
    }
    memset(sheet, 0x40, sizeof sheet);
    for (int k = 0; k < shot_count; k++) {
        int w = shots[k].landscape ? H : W, h = shots[k].landscape ? W : H;
        canvas_t c = {pixels, w, h, 0, h};
        shots[k].draw(&c);
        // Centered in its square cell.
        int top = k / SHEET_COLUMNS * (H + GAP) + (H - h) / 2, left = k % SHEET_COLUMNS * (H + GAP) + (H - w) / 2;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                rgb565_to_rgb888(pixels[y * w + x], shot[k][y * w + x]);
                memcpy(sheet[top + y][left + x], shot[k][y * w + x], 3);
            }
        }
        if (save_ppm(argv[1], shots[k].name, &shot[k][0][0], w, h)) {
            return 1;
        }
    }
    compose_showcase();
    int rows = (shot_count + SHEET_COLUMNS - 1) / SHEET_COLUMNS;
    return save_ppm(argv[1], "sheet", &sheet[0][0][0], SHEET_W, rows * (H + GAP) - GAP) ||
           save_ppm(argv[1], "showcase", &showcase[0][0][0], SHOWCASE_W, SHOWCASE_H);
}
