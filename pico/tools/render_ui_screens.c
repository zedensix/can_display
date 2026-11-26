// The screens render_ui draws other than the gauges: the CAN self test's
// failure and wait, Send log, the settings, the boot, the screensaver, the
// doors, each from a fixed state.

#include <stdio.h>
#include <string.h>

#include "obd/mcp_check.h"
#include "render_ui.h"

// The screensaver, on the 86's logo (the parts' logos stay out of git):
// the shine across it, then fading in.
static void shot_showcar(canvas_t *c) {
    state_t s = {0};
    snprintf(s.status, sizeof s.status, "%s", mcp_bus_status(BUS_SILENT));
    ui_render(c, &s, &(screen_t){.idle = true}, &cfg, 1300);
}

static void shot_showcar_later(canvas_t *c) {
    state_t s = {0};
    snprintf(s.status, sizeof s.status, "%s", mcp_bus_status(BUS_SILENT));
    ui_render(c, &s, &(screen_t){.idle = true}, &cfg, 300);
}

// The MCP2515's self test: its longest failure, and a silent bus.
static void shot_can_failed(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", mcp_check_spi(0x00, false));
    render_screen(c, &s, (screen_t){0});
}

static void shot_can_silent(canvas_t *c) {
    state_t s = {0};
    snprintf(s.status, sizeof s.status, "%s", mcp_bus_status(BUS_SILENT));
    render_screen(c, &s, (screen_t){0});
}

// Send log, halfway through.
static void shot_send_log(canvas_t *c) {
    state_t s = {.status = "SEND LOG\n412 of 950 KB"};
    render_screen(c, &s, (screen_t){0});
}

// The driver's door open; the passenger's and the boot.
static void shot_door(canvas_t *c) {
    state_t s = state_while_driving();
    state_set_doors(&s, DOOR_DRIVER, now);
    render_screen(c, &s, (screen_t){0});
}

static void shot_doors(canvas_t *c) {
    state_t s = state_while_driving();
    state_set_doors(&s, DOOR_PASSENGER | DOOR_BOOT, now);
    render_screen(c, &s, (screen_t){0});
}

static void shot_settings(canvas_t *c) {
    state_t s = {0};
    render_screen(c, &s, (screen_t){.page = PAGE_SETTINGS, .cursor = ROW_SHIFT_LIGHT, .editing = true});
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

const shot_t screen_shots[] = {
    {"can_failed", shot_can_failed},
    {"can_silent", shot_can_silent},
    {"send_log", shot_send_log},
    {"door", shot_door},
    {"doors", shot_doors},
    {"showcar", shot_showcar},
    {"showcar_later", shot_showcar_later},
    {"settings", shot_settings},
    {"shot_boot_spin", shot_boot_spin},
    {"shot_boot_morph", shot_boot_morph},
    {"shot_boot_logo", shot_boot_logo},
};
const int screen_shot_count = sizeof screen_shots / sizeof screen_shots[0];
