// The screens render_ui draws other than the gauges: the data source's
// prompts and failures, the settings, the boot, the screensaver, the
// doors, each from a fixed state.

#include <stdio.h>
#include <string.h>

#include "obd/mcp_check.h"
#include "render_ui.h"

// A prompt waiting for the driver's choice, the knob on choice.
static void render_choice_screen(canvas_t *c, const char *text, choice_t choice) {
    state_t s = {.wait = WAIT_CHOICE};
    snprintf(s.status, sizeof s.status, "%s", text);
    render_screen(c, &s, (screen_t){.choice = choice});
}

static void shot_ready(canvas_t *c) {
    render_choice_screen(c, "READY\nIOS-Vlink\n(last used)", CHOICE_CONNECT);
}

// The dongle asleep, the car off: no failure, a click reconnects.
static void shot_asleep(canvas_t *c) {
    render_choice_screen(c, "DISCONNECTED\npress to reconnect", CHOICE_CONNECT);
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

// The longest title, one word, as ble.c words it.
static void shot_handshake(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY, .status = "HANDSHAKE\n41:42:86:9A:C0:33"};
    render_screen(c, &s, (screen_t){0});
}

static void shot_connecting(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", "LINKING\n41:42:86:9A:C0:33");
    render_screen(c, &s, (screen_t){0});
}

static void shot_failed(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s",
             "NO LINK\nBLE connect failed:\ncan't connect: the radio still holds another connection (0x56)");
    render_screen(c, &s, (screen_t){.choice = CHOICE_SETTINGS});
}
// The MCP2515's self test: its longest failure, and a silent bus.
static void shot_can_failed(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", mcp_check_spi(0x00, false));
    render_screen(c, &s, (screen_t){0});
}

static void shot_can_silent(canvas_t *c) {
    state_t s = {.wait = WAIT_RETRY};
    snprintf(s.status, sizeof s.status, "%s", mcp_bus_status(BUS_SILENT));
    render_screen(c, &s, (screen_t){0});
}

// A listen's prompt, Next and Cancel; then listening, Cancel.
static void shot_listen(canvas_t *c) {
    state_t s = {.wait = WAIT_STEP,
                 .status = "LISTEN 8/15\nhold the cruise stalk UP\nuntil the next prompt\nthen Next"};
    render_screen(c, &s, (screen_t){.choice = CHOICE_CONNECT});
}

static void shot_listening(canvas_t *c) {
    state_t s = {.wait = WAIT_CANCEL, .status = "LISTEN 8/15\nkeep it so\n43%"};
    render_screen(c, &s, (screen_t){.choice = CHOICE_CONNECT});
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

// A RAM scan, its Cancel chosen.
static void shot_scan(canvas_t *c) {
    state_t s = {.wait = WAIT_CANCEL, .status = "DEEP SCAN\n37 KB of ECU RAM\n3 candidates"};
    render_screen(c, &s, (screen_t){.choice = CHOICE_CONNECT});
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

const shot_t screen_shots[] = {
    {"ready", shot_ready, false},
    {"offline", shot_offline, false},
    {"asleep", shot_asleep, false},
    {"connecting", shot_connecting, false},
    {"scan", shot_scan, false},
    {"listen", shot_listen, false},
    {"door", shot_door, false},
    {"doors", shot_doors, false},
    {"listening", shot_listening, false},
    {"failed", shot_failed, false},
    {"can_failed", shot_can_failed, false},
    {"can_silent", shot_can_silent, false},
    {"showcar", shot_showcar, false},
    {"showcar_later", shot_showcar_later, false},
    {"settings", shot_settings, false},
    {"settings_can", shot_settings_can, false},
    {"shot_boot_spin", shot_boot_spin, false},
    {"shot_boot_morph", shot_boot_morph, false},
    {"shot_boot_logo", shot_boot_logo, false},
    {"ready_landscape", shot_ready, true},
    {"handshake_landscape", shot_handshake, true},
    {"asleep_landscape", shot_asleep, true},
    {"failed_landscape", shot_failed, true},
    {"scan_landscape", shot_scan, true},
    {"listen_landscape", shot_listen, true},
    {"door_landscape", shot_door, true},
    {"settings_landscape", shot_settings, true},
    {"showcar_landscape", shot_showcar, true},
    {"boot_landscape", shot_boot_morph, true},
};
const int screen_shot_count = sizeof screen_shots / sizeof screen_shots[0];
