// A new firmware from the phone (the app's own, bundled in it): it asks
// with 'U' (size, CRC-32), the driver allows it with a click on the knob,
// the image comes in 'W' writes staged in the data log's ring (update.h),
// then the firmware copies it over itself and restarts. Its replies are
// notifications of the update characteristic:
//   'A' u32 n  the dash has n bytes and wants those from n on (0: go);
//              the phone sends at most a sector past it
//   'I'        all there with its CRC: installing, the dash restarts
//   'E' u8     over: 1 refused (no click within ASK_MS, cancelled on the
//              dash, busy), 2 no room for it, 3 CRC mismatch, 4 the phone
//              silent for QUIET_MS
// The update characteristic read gives the running firmware's CRC-32 and
// size, u32 each, for the phone to tell whether its own is newer.

#include "ble_internal.h"

#include <stdio.h>

#include "btstack.h"
#include "hardware/store.h"

#define ASK_MS 30000
#define QUIET_MS 10000
// From 'I' to the copy: the notification out, "installing" on the screen.
#define INSTALL_DELAY_MS 500

typedef enum { IDLE, ASKING, RECEIVING, INSTALLING } stage_t;

static stage_t stage;
static uint16_t update_handle;
static update_t update;
static state_t *state;
static hci_con_handle_t phone = HCI_CON_HANDLE_INVALID;
static uint32_t size, crc, since, resynced_at;
static uint8_t reply[5];
static uint16_t reply_size;
static char status_before[sizeof((state_t *)0)->status];
static wait_t wait_before;

static void send_reply(void) {
    if (reply_size && phone != HCI_CON_HANDLE_INVALID) {
        if (att_server_notify(phone, update_handle, reply, reply_size) == ERROR_CODE_SUCCESS) {
            reply_size = 0;
        } else {
            att_server_request_can_send_now_event(phone);
        }
    }
}

// queue_reply replaces the reply not sent yet: the latest says it all.
static void queue_reply(uint8_t kind, uint32_t value, uint16_t value_size) {
    reply[0] = kind;
    little_endian_store_32(reply, 1, value);
    reply_size = 1 + value_size;
    send_reply();
}

static void show(const char *status) {
    state_set_status(state, status);
}

static void show_progress(void) {
    char status[96];
    snprintf(status, sizeof status, "UPDATE\n%lu of %lu KB\npress: cancel", (unsigned long)(update.received / 1024),
             (unsigned long)(size / 1024));
    show(status);
}

// finish ends the update, the screen as it was; an error code tells the
// phone why, the image's sectors written so far erased.
static void finish(uint8_t error) {
    if (stage == RECEIVING) {
        update_erase(&update, false);
    }
    if (error) {
        queue_reply('E', error, 1);
        log_line("update: over, error %u", error);
    }
    stage = IDLE;
    state_set_status(state, status_before);
    state->wait = wait_before;
}

static void ask(hci_con_handle_t con, const uint8_t *command, uint16_t length, uint32_t now, bool busy) {
    phone = con;
    if (length < 9 || stage != IDLE || busy || !store_get_datalog()) {
        queue_reply('E', 1, 1);
        return;
    }
    size = little_endian_read_32(command, 1);
    crc = little_endian_read_32(command, 5);
    if (size == 0 || size > STORE_FIRMWARE_ROOM) {
        queue_reply('E', 2, 1);
        return;
    }
    log_line("update: the phone offers %lu bytes", (unsigned long)size);
    snprintf(status_before, sizeof status_before, "%s", state->status);
    wait_before = state->wait;
    state->wait = WAIT_NONE;
    char status[96];
    snprintf(status, sizeof status, "UPDATE\nnew firmware\nfrom the phone, %lu KB\npress: install",
             (unsigned long)(size / 1024));
    show(status);
    stage = ASKING;
    since = now;
}

// start stages the image once the driver allowed it: the data log
// written out first, its ring's oldest sectors then taken.
static void start(uint32_t now) {
    datalog_t *log = store_get_datalog();
    datalog_flush(log);
    if (!update_begin(&update, log->dev, log->pos, size, crc)) {
        finish(2);
        return;
    }
    stage = RECEIVING;
    since = now;
    resynced_at = UINT32_MAX;
    show_progress();
    queue_reply('A', 0, 4);
}

static void take(const uint8_t *command, uint16_t length, uint32_t now) {
    if (stage != RECEIVING || length < 5) {
        return;
    }
    since = now;
    uint32_t offset = little_endian_read_32(command, 1), before = update.received;
    if (!update_write(&update, offset, command + 5, length - 5u)) {
        // A write lost before this one: the phone goes back to where the
        // dash is, told once per gap. Those it sent again are dropped.
        if (offset > update.received && resynced_at != update.received) {
            resynced_at = update.received;
            queue_reply('A', update.received, 4);
        }
        return;
    }
    if (update.received < size) {
        if (update.received / FLASHLOG_SECTOR != before / FLASHLOG_SECTOR) {
            queue_reply('A', update.received, 4);
            show_progress();
        }
        return;
    }
    if (!update_check(&update)) {
        finish(3);
        return;
    }
    log_line("update: image checked");
    queue_reply('I', 0, 0);
    show("UPDATE\ninstalling\ndon't turn the car off");
    stage = INSTALLING;
    since = now;
}

void ble_update_start(state_t *s, uint16_t handle) {
    state = s;
    update_handle = handle;
}

bool ble_update_command(hci_con_handle_t con, const uint8_t *command, uint16_t length, uint32_t now, bool busy) {
    switch (command[0]) {
    case 'U':
        ask(con, command, length, now, busy);
        return true;
    case 'W':
        take(command, length, now);
        return true;
    case 'X':
        if (stage == ASKING || stage == RECEIVING) {
            log_line("update: cancelled on the phone");
            finish(0);
        }
        return true;
    default:
        return false;
    }
}

uint16_t ble_update_read(uint16_t handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size) {
    if (handle != update_handle) {
        return 0;
    }
    uint8_t version[8];
    uint32_t firmware_size;
    little_endian_store_32(version, 0, store_firmware_crc(&firmware_size));
    little_endian_store_32(version, 4, firmware_size);
    return att_read_callback_handle_blob(version, sizeof version, offset, buffer, buffer_size);
}

void ble_update_can_send(void) {
    send_reply();
}

void ble_update_disconnected(void) {
    phone = HCI_CON_HANDLE_INVALID;
    reply_size = 0;
    // Installing needs the phone no more.
    if (stage == ASKING || stage == RECEIVING) {
        log_line("update: the phone left");
        finish(0);
    }
}

void ble_update_poll(uint32_t now) {
    if (stage == ASKING && now - since > ASK_MS) {
        finish(1);
    } else if (stage == RECEIVING && now - since > QUIET_MS) {
        finish(4);
    } else if (stage == INSTALLING && now - since > INSTALL_DELAY_MS) {
        store_install(&update);
    }
}

bool ble_update_click(uint32_t now) {
    switch (stage) {
    case ASKING:
        start(now);
        return true;
    case RECEIVING:
        log_line("update: cancelled on the dash");
        finish(1);
        return true;
    case INSTALLING:
        return true;
    default:
        return false;
    }
}

bool ble_updating(void) {
    return stage != IDLE;
}
