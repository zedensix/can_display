#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui_internal.h"

extern const unsigned char logo_bin[];

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
void draw_spinner(canvas_t *c, const theme_t *t, int cx, int cy, uint32_t now_ms, float done) {
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

#define CHOICE_HEIGHT 26

// draw_choice_buttons draws Connect and Settings along the bottom, the
// one a click does filled.
static void draw_choice_buttons(canvas_t *c, const theme_t *t, choice_t choice) {
    static const char *const labels[] = {"CONNECT", "SETTINGS"};
    int gap = 8, w = (c->width - 2 * STATUS_MARGIN - gap) / 2, y = c->height - CHOICE_HEIGHT - 8;
    for (int i = 0; i < 2; i++) {
        int x = STATUS_MARGIN + i * (w + gap);
        bool chosen = (int)choice == i;
        if (chosen) {
            gfx_fill_rect(c, x, y, w, CHOICE_HEIGHT, t->accent);
        } else {
            draw_outline(c, x, y, w, CHOICE_HEIGHT, gfx_blend(t->background, t->accent, 0.5f));
        }
        draw_text_centered(c, label_font, x + w / 2, y + 18, labels[i], chosen ? t->background : t->accent, 1);
    }
}

// draw_status_screen is the source's report while it isn't delivering: the
// logo in a spinner (so a slow step doesn't look frozen), the first
// paragraph as a title, the rest centered and wrapped under it. Waiting
// for a choice, Connect and Settings along the bottom.
void draw_status_screen(canvas_t *c, const theme_t *t, const state_t *state, const screen_t *screen, uint32_t now_ms) {
    draw_spinner(c, t, c->width / 2, RING_Y, now_ms, 0);
    draw_logo(c, c->width / 2, RING_Y, 2, t->accent);
    bool choosing = state->wait == WAIT_CHOICE;
    if (choosing) {
        draw_choice_buttons(c, t, screen->choice);
    }

    int y = RING_Y + RING_OUT + 34, bottom = choosing ? c->height - CHOICE_HEIGHT - 12 : c->height;
    bool first = true;
    char paragraph[192];
    snprintf(paragraph, sizeof paragraph, "%s", state->status);
    for (char *p = paragraph; p && y < bottom; first = false) {
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

void ui_spinner_rows(int panel_height, bool boot, int *top, int *rows) {
    int center = boot ? panel_height / 2 : RING_Y;
    *top = center - RING_OUT - 1;
    *rows = 2 * RING_OUT + 3;
}

void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color) {
    draw_logo_shine(c, cx, cy, shrink, color, color, -1000);
}

#define SHINE_BAND 22

void draw_logo_shine(canvas_t *c, int cx, int cy, int shrink, color_t color, color_t shine, int sweep) {
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
            if (!on) {
                continue;
            }
            int d = x0 + x + y0 + y - sweep;
            d = d < 0 ? -d : d;
            color_t lit = d < SHINE_BAND ? blend_rgb565(color, shine, (unsigned)(255 - d * 255 / SHINE_BAND)) : color;
            gfx_fill_rect(c, x0 + x, y0 + y, 1, 1, lit);
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
