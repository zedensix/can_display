#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ui_internal.h"

int ui_wrap_text(const font_t *font, int spacing, const char *s, int max_width, char out[][64], int max) {
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
        bool too_wide = len >= (int)sizeof candidate || gfx_text_width(font, candidate, spacing) > max_width;
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
// The title (the status's first paragraph) as the Android app's waiting
// screen has it: white, bold, letter-spaced; the rest under it, dim.
#define TITLE_FONT (&font_value)
#define TITLE_SPACING 2
#define TITLE_EXTRA 12
#define LINE_FONT (&font_text)
#define STATUS_LINE_HEIGHT 25
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
            color_t color = gfx_mix(t->background, gfx_mix(t->stale, t->accent, lit), p->alpha);
            gfx_fill_rect(c, cx + sx[q] * p->dx, cy + sy[q] * p->dy, 1, 1, color);
        }
    }
}

#define CHOICE_HEIGHT 30

// draw_choice_buttons draws Retry and Settings along the bottom, the one
// a click does filled.
static void draw_choice_buttons(canvas_t *c, const theme_t *t, choice_t choice) {
    int gap = 8, w = (c->width - 2 * STATUS_MARGIN - gap) / 2, y = c->height - CHOICE_HEIGHT - 8;
    draw_button(c, t, STATUS_MARGIN, y, w, CHOICE_HEIGHT, ICON_RETRY, "Retry", choice == CHOICE_RETRY);
    draw_button(c, t, STATUS_MARGIN + w + gap, y, w, CHOICE_HEIGHT, ICON_GEAR, "Settings", choice == CHOICE_SETTINGS);
}

// draw_status_screen is the source's report while it isn't delivering: the
// logo in a spinner (so a slow step doesn't look frozen), the first
// paragraph as a title, the rest centered and wrapped under it (in
// landscape, beside it). While it waits on the driver, Retry and Settings
// along the bottom.
#define STATUS_MAX_LINES 14
// The compact layout's logo, above the text, when the text needs the room.
#define COMPACT_LOGO_Y 44
#define COMPACT_TEXT_Y 110

// wrap_status wraps status to width: its title's lines then the rest's
// in lines, title telling which. It returns how many, and their height.
static int wrap_status(const char *status, int width, char lines[][64], bool title[], int *height) {
    int n = 0;
    *height = 0;
    char paragraph[192];
    snprintf(paragraph, sizeof paragraph, "%s", status);
    for (char *p = paragraph; p && n < STATUS_MAX_LINES;) {
        char *next = strchr(p, '\n');
        if (next) {
            *next++ = 0;
        }
        bool first = n == 0;
        int added = ui_wrap_text(first ? TITLE_FONT : LINE_FONT, first ? TITLE_SPACING : 0, p, width, &lines[n],
                                 STATUS_MAX_LINES - n);
        for (int i = n; i < n + added; i++) {
            title[i] = first;
            *height += STATUS_LINE_HEIGHT + (first ? TITLE_EXTRA : 0);
        }
        n += added;
        p = next;
    }
    return n;
}

void draw_status_screen(canvas_t *c, const theme_t *t, const state_t *state, const screen_t *screen, uint32_t now_ms) {
    bool choosing = state->wait != WAIT_NONE, side = is_landscape(c);
    int bottom = choosing ? c->height - CHOICE_HEIGHT - 12 : c->height;
    // Portrait: the ring centered, the text under it. Landscape: the ring
    // on the left, the text in a column on its right, around RING_Y.
    int ring_x = side ? STATUS_MARGIN + RING_OUT + 8 : c->width / 2;
    int text_left = side ? ring_x + RING_OUT + 16 : STATUS_MARGIN, text_w = c->width - STATUS_MARGIN - text_left;

    char lines[STATUS_MAX_LINES][64];
    bool title[STATUS_MAX_LINES];
    int height, n = wrap_status(state->status, text_w, lines, title, &height);
    int y = side ? RING_Y - height / 2 + STATUS_LINE_HEIGHT - 4 : RING_Y + RING_OUT + 34;
    // A word wider than the text's column can't wrap: it doesn't fit.
    bool too_wide = false;
    for (int i = 0; i < n; i++) {
        too_wide = too_wide ||
                   gfx_text_width(title[i] ? TITLE_FONT : LINE_FONT, lines[i], title[i] ? TITLE_SPACING : 0) > text_w;
    }
    // A long text (an error over the buttons), or one too wide, gets the
    // whole width instead, under a small logo.
    if (too_wide || y + height - STATUS_LINE_HEIGHT > bottom || y < STATUS_LINE_HEIGHT) {
        text_left = STATUS_MARGIN;
        text_w = c->width - 2 * STATUS_MARGIN;
        n = wrap_status(state->status, text_w, lines, title, &height);
        draw_logo(c, c->width / 2, COMPACT_LOGO_Y, 3, t->accent);
        y = COMPACT_TEXT_Y;
    } else {
        draw_spinner(c, t, ring_x, RING_Y, now_ms, 0);
        draw_logo(c, ring_x, RING_Y, 2, t->accent);
    }
    if (choosing) {
        draw_choice_buttons(c, t, screen->choice);
    }
    for (int i = 0; i < n && y <= bottom; i++) {
        draw_text_centered(c, title[i] ? TITLE_FONT : LINE_FONT, text_left + text_w / 2, y, lines[i],
                           title[i] ? t->label : t->dim, title[i] ? TITLE_SPACING : 0);
        y += STATUS_LINE_HEIGHT + (title[i] ? TITLE_EXTRA : 0);
    }
}

void ui_spinner_rows(int panel_height, bool boot, int *top, int *rows) {
    int center = boot ? panel_height / 2 : RING_Y;
    *top = center - RING_OUT - 1;
    *rows = 2 * RING_OUT + 3;
}

void draw_logo(canvas_t *c, int cx, int cy, int shrink, color_t color) {
    draw_bitmap(c, logo_bin, cx, cy, shrink, color, color, -1000);
}

#define SHINE_BAND 22

void draw_bitmap(canvas_t *c, const unsigned char *bitmap, int cx, int cy, int shrink, color_t color, color_t shine,
                 int sweep) {
    int w = bitmap[0] << 8 | bitmap[1], h = bitmap[2] << 8 | bitmap[3];
    int stride = (w + 7) / 8;
    int x0 = cx - w / shrink / 2, y0 = cy - h / shrink / 2;
    const unsigned char *rows = bitmap + 4;
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
            color_t lit = d < SHINE_BAND ? gfx_mix(color, shine, (unsigned)(255 - d * 255 / SHINE_BAND)) : color;
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
