#include "ble.h"

#include <stdio.h>

#include "ble_internal.h"
#include "btstack.h"
#include "dash.h"
#include "data/live.h"
#include "hardware/store.h"
#include "pico/cyw43_arch.h"

// The data log on its way to a phone: the part from first on, total
// bytes after a header: for Send log ('S') total alone, 4 bytes; for a
// pull ('R', what's new since a position the phone kept, a 1 after it
// for the one before an update) 18 bytes: total,
// where the part starts in flash, where the log is now (the next pull's
// position), the dash's milliseconds since boot and its session, the
// first four u32 and the last a u16, little-endian.
#define DATA_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
#define DATA_CCCD_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_CLIENT_CONFIGURATION_HANDLE
#define CONTROL_HANDLE ATT_CHARACTERISTIC_c0ff8601_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
// The live readings, a frame every LIVE_MS to a phone that subscribed.
#define LIVE_HANDLE ATT_CHARACTERISTIC_c0ff8603_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
#define LIVE_CCCD_HANDLE ATT_CHARACTERISTIC_c0ff8603_4b1e_4c7a_9e2d_5a3f86d0a001_01_CLIENT_CONFIGURATION_HANDLE
#define LIVE_MS 50
// From the phone's 'D' to the gauges back, for "downloaded" to be read.
#define RESUME_MS 2500

static state_t *state;
static datalog_t *log_to_send;
// The log being read out, Send log's or a pull's.
static const datalog_t *reading;
static uint8_t header[18];
static uint16_t header_size;
static bool stack_up;
static btstack_packet_callback_registration_t hci_registration;
static hci_con_handle_t phone = HCI_CON_HANDLE_INVALID;
static bool streaming, header_sent, live, update_pull;
static uint32_t first, total, sent, now, received_at, live_at;
// What the screen showed before Send log, put back after it.
static char status_before[sizeof((state_t *)0)->status];
static wait_t wait_before;

static void advertise(void) {
    // Flags, then the service's UUID (bytes reversed) for the phone to
    // filter on; the name goes in the scan response.
    // clang-format off: bytes as BLE packs them, one field per line.
    static uint8_t adv[] = {0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06, 0x11,
                            BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x01, 0xa0, 0xd0, 0x86,
                            0x3f, 0x5a, 0x2d, 0x9e, 0x7a, 0x4c, 0x1e, 0x4b, 0x00, 0x86, 0xff, 0xc0};
    static uint8_t scan_response[] = {0x0A, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'G', 'T', '8', '6', ' ',
                                      'D', 'A', 'S', 'H'};
    // clang-format on
    bd_addr_t anyone = {0};
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, anyone, 0x07, 0x00);
    gap_advertisements_set_data(sizeof adv, adv);
    gap_scan_response_set_data(sizeof scan_response, scan_response);
    gap_advertisements_enable(1);
}

static void send_next_log_chunk(void) {
    if (!streaming || phone == HCI_CON_HANDLE_INVALID) {
        return;
    }
    // A notification refused (no buffer free: the live readings share
    // them) goes again at the next chance, not skipped.
    if (!header_sent) {
        if (att_server_notify(phone, DATA_HANDLE, header, header_size) != ERROR_CODE_SUCCESS) {
            att_server_request_can_send_now_event(phone);
            return;
        }
        header_sent = true;
    } else {
        uint8_t chunk[244];
        uint32_t n = att_server_get_mtu(phone) - 3;
        n = n > sizeof chunk ? sizeof chunk : n;
        n = n > total - sent ? total - sent : n;
        datalog_read(reading, first + sent, chunk, n);
        if (att_server_notify(phone, DATA_HANDLE, chunk, (uint16_t)n) != ERROR_CODE_SUCCESS) {
            att_server_request_can_send_now_event(phone);
            return;
        }
        uint32_t before = sent;
        sent += n;
        if (log_to_send && sent / 8192 != before / 8192) {
            char status[96];
            snprintf(status, sizeof status, "SEND LOG\n%lu of %lu KB", (unsigned long)(sent / 1024),
                     (unsigned long)(total / 1024));
            state_set_status(state, status);
        } else if (update_pull && sent / 8192 != before / 8192) {
            ble_update_pulling(sent, total, now);
        }
    }
    if (sent < total) {
        att_server_request_can_send_now_event(phone);
        return;
    }
    streaming = false;
    if (update_pull) {
        ble_update_pulling(total, total, now);
    }
    if (!log_to_send) {
        return;
    }
    log_line("ble: data log sent, %lu bytes", (unsigned long)total);
    char status[96];
    snprintf(status, sizeof status, "SEND LOG\nsent %lu KB\nwaiting for the phone\npress: back",
             (unsigned long)(total / 1024));
    state_set_status(state, status);
}

// pull sends what the data log holds since from, while it goes on
// logging: the app's periodic pull, the gauges never paused. Not during
// Send log, which has the log, nor during another pull.
static void pull(hci_con_handle_t con, uint32_t from, bool for_update) {
    datalog_t *log = store_get_datalog();
    if (!log || log_to_send || streaming || ble_updating()) {
        return;
    }
    datalog_flush(log);
    uint32_t next = datalog_since(log, from, &first, &total);
    reading = log;
    little_endian_store_32(header, 0, total);
    little_endian_store_32(header, 4, (log->pos + first) % log->dev.size);
    little_endian_store_32(header, 8, next);
    little_endian_store_32(header, 12, now);
    little_endian_store_16(header, 16, log->session);
    header_size = 18;
    sent = 0;
    header_sent = false;
    streaming = true;
    update_pull = for_update;
    if (update_pull) {
        ble_update_pulling(0, total, now);
    }
    att_server_request_can_send_now_event(con);
}

static uint16_t handle_att_read(hci_con_handle_t con, uint16_t handle, uint16_t offset, uint8_t *buffer,
                                uint16_t buffer_size) {
    (void)con;
    return ble_update_read(handle, offset, buffer, buffer_size);
}

static int handle_att_write(hci_con_handle_t con, uint16_t handle, uint16_t mode, uint16_t offset, uint8_t *buffer,
                            uint16_t size) {
    (void)mode;
    (void)offset;
    if (handle == LIVE_CCCD_HANDLE && size >= 1) {
        live = buffer[0] & GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
        log_line("ble: live readings %s", live ? "on" : "off");
        return 0;
    }
    if (handle != CONTROL_HANDLE || size < 1) {
        return 0;
    }
    if (ble_update_command(con, buffer, size, now, log_to_send || streaming)) {
        return 0;
    }
    if (buffer[0] == 'R' && size >= 5) {
        pull(con, little_endian_read_32(buffer, 1), size >= 6 && buffer[5] == 1);
        return 0;
    }
    if (!log_to_send) {
        return 0;
    }
    if (buffer[0] == 'D') {
        log_line("ble: the phone got the data log");
        state_set_status(state, "SEND LOG\ndownloaded\nback to the gauges");
        received_at = now ? now : 1;
    } else if (buffer[0] == 'S') {
        reading = log_to_send;
        first = datalog_oldest_record_offset(log_to_send);
        total = log_to_send->dev.size - first;
        little_endian_store_32(header, 0, total);
        header_size = 4;
        sent = 0;
        header_sent = false;
        streaming = true;
        log_line("ble: sending the data log, %lu bytes", (unsigned long)total);
        att_server_request_can_send_now_event(con);
    }
    return 0;
}

static void handle_att_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type == HCI_EVENT_PACKET && hci_event_packet_get_type(packet) == ATT_EVENT_CAN_SEND_NOW) {
        ble_update_can_send();
        send_next_log_chunk();
    }
}

static void handle_hci_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
            advertise();
        }
        break;
    case HCI_EVENT_META_GAP:
        // Covers both the legacy and the enhanced (BT 5) connection events.
        if (hci_event_gap_meta_get_subevent_code(packet) == GAP_SUBEVENT_LE_CONNECTION_COMPLETE &&
            gap_subevent_le_connection_complete_get_status(packet) == ERROR_CODE_SUCCESS) {
            phone = gap_subevent_le_connection_complete_get_connection_handle(packet);
            log_line("ble: phone connected");
            if (log_to_send) {
                state_set_status(state, "SEND LOG\nphone connected");
            }
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        // The phone gone: back on the air for the next one.
        phone = HCI_CON_HANDLE_INVALID;
        streaming = false;
        live = false;
        ble_update_disconnected();
        advertise();
        break;
    default:
        break;
    }
}

void ble_start(state_t *s) {
    state = s;
    ble_update_start(s, ATT_CHARACTERISTIC_c0ff8604_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE);
    if (cyw43_arch_init()) {
        log_line("ble: cyw43 init failed");
        return;
    }
    stack_up = true;
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    att_server_init(profile_data, handle_att_read, handle_att_write);
    att_server_register_packet_handler(handle_att_event);
    hci_registration.callback = handle_hci_event;
    hci_add_event_handler(&hci_registration);
    // The HCI working event puts the dash on the air.
    hci_power_control(HCI_POWER_ON);
}

void ble_send_log(datalog_t *log) {
    if (!log || log_to_send || ble_updating()) {
        return;
    }
    log_line("ble: send log");
    log_to_send = log;
    received_at = 0;
    snprintf(status_before, sizeof status_before, "%s", state->status);
    wait_before = state->wait;
    state->wait = WAIT_NONE;
    state_set_status(state, stack_up ? "SEND LOG\nopen the page\non your phone\npress: back"
                                     : "SEND LOG\nBluetooth didn't start\npress: back");
}

// stop_sending ends Send log: the screen as it was.
static void stop_sending(void) {
    log_line("ble: send log over");
    log_to_send = NULL;
    streaming = false;
    state_set_status(state, status_before);
    state->wait = wait_before;
}

bool ble_sending(void) {
    return log_to_send != NULL || ble_updating();
}

void ble_poll_radio(uint32_t now_ms) {
    now = now_ms;
    if (!stack_up) {
        return;
    }
    cyw43_arch_poll();
    ble_update_poll(now);
    if (log_to_send && received_at && now - received_at > RESUME_MS) {
        stop_sending();
    }
    // A frame dropped for want of a buffer is no loss: the next one comes
    // LIVE_MS later. They go on through a pull or Send log, sent only
    // when a buffer is free.
    if (live && phone != HCI_CON_HANDLE_INVALID && now - live_at >= LIVE_MS && att_server_can_send_packet_now(phone)) {
        live_at = now;
        uint8_t frame[LIVE_SIZE];
        live_encode(state, now, frame);
        att_server_notify(phone, LIVE_HANDLE, frame, sizeof frame);
    }
}

bool ble_handle_click(void) {
    if (ble_update_click(now)) {
        return true;
    }
    if (!log_to_send) {
        return false;
    }
    stop_sending();
    return true;
}
