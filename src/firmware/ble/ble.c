#include "ble.h"

#include <stdio.h>

#include "btstack.h"
#include "dash.h"
#include "hardware/store.h"
#include "pico/cyw43_arch.h"

// The data log on its way to a phone: the part from first on, total
// bytes after a 4-byte header holding total.
#define DATA_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
#define DATA_CCCD_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_CLIENT_CONFIGURATION_HANDLE
#define CONTROL_HANDLE ATT_CHARACTERISTIC_c0ff8601_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
// From the phone's 'D' to the gauges back, for "downloaded" to be read.
#define RESUME_MS 2500

static state_t *state;
static datalog_t *log_to_send;
static bool stack_up;
static btstack_packet_callback_registration_t hci_registration;
static hci_con_handle_t phone = HCI_CON_HANDLE_INVALID;
static bool streaming, header_sent;
static uint32_t first, total, sent, now, received_at;
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
    state_set_status(state, "SEND LOG\nopen the page\non your phone\npress: back");
}

static void send_next_log_chunk(void) {
    if (!streaming || phone == HCI_CON_HANDLE_INVALID) {
        return;
    }
    if (!header_sent) {
        uint8_t header[4];
        little_endian_store_32(header, 0, total);
        att_server_notify(phone, DATA_HANDLE, header, sizeof header);
        header_sent = true;
    } else {
        uint8_t chunk[244];
        uint32_t n = att_server_get_mtu(phone) - 3;
        n = n > sizeof chunk ? sizeof chunk : n;
        n = n > total - sent ? total - sent : n;
        datalog_read(log_to_send, first + sent, chunk, n);
        att_server_notify(phone, DATA_HANDLE, chunk, (uint16_t)n);
        uint32_t before = sent;
        sent += n;
        if (sent / 8192 != before / 8192) {
            char status[96];
            snprintf(status, sizeof status, "SEND LOG\n%lu of %lu KB", (unsigned long)(sent / 1024),
                     (unsigned long)(total / 1024));
            state_set_status(state, status);
        }
    }
    if (sent < total) {
        att_server_request_can_send_now_event(phone);
        return;
    }
    streaming = false;
    log_line("ble: data log sent, %lu bytes", (unsigned long)total);
    char status[96];
    snprintf(status, sizeof status, "SEND LOG\nsent %lu KB\nwaiting for the phone\npress: back",
             (unsigned long)(total / 1024));
    state_set_status(state, status);
}

static int handle_att_write(hci_con_handle_t con, uint16_t handle, uint16_t mode, uint16_t offset, uint8_t *buffer,
                            uint16_t size) {
    (void)mode;
    (void)offset;
    if (handle != CONTROL_HANDLE || size < 1 || !log_to_send) {
        return 0;
    }
    if (buffer[0] == 'D') {
        log_line("ble: the phone got the data log");
        state_set_status(state, "SEND LOG\ndownloaded\nback to the gauges");
        received_at = now ? now : 1;
    } else if (buffer[0] == 'S') {
        first = datalog_oldest_record_offset(log_to_send);
        total = log_to_send->dev.size - first;
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
        send_next_log_chunk();
    }
}

static void handle_hci_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET || !log_to_send) {
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
            state_set_status(state, "SEND LOG\nphone connected");
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        // The phone gone before it got it all: back on the air for it.
        phone = HCI_CON_HANDLE_INVALID;
        streaming = false;
        if (!received_at) {
            advertise();
        }
        break;
    default:
        break;
    }
}

// start_radio brings the radio and BTstack up the first time, powers the
// radio on after; the HCI working event puts the dash on the air.
static bool start_radio(void) {
    if (stack_up) {
        hci_power_control(HCI_POWER_ON);
        return true;
    }
    if (cyw43_arch_init()) {
        return false;
    }
    stack_up = true;
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    att_server_init(profile_data, NULL, handle_att_write);
    att_server_register_packet_handler(handle_att_event);
    hci_registration.callback = handle_hci_event;
    hci_add_event_handler(&hci_registration);
    hci_power_control(HCI_POWER_ON);
    return true;
}

void ble_send_log(state_t *s, datalog_t *log) {
    if (!log || log_to_send) {
        return;
    }
    log_line("ble: send log");
    state = s;
    log_to_send = log;
    received_at = 0;
    phone = HCI_CON_HANDLE_INVALID;
    snprintf(status_before, sizeof status_before, "%s", s->status);
    wait_before = s->wait;
    s->wait = WAIT_NONE;
    state_set_status(s, "SEND LOG\nstarting Bluetooth");
    store_hold_core1(true);
    if (!start_radio()) {
        log_line("ble: cyw43 init failed");
        state_set_status(s, "SEND LOG\nBluetooth didn't start\npress: back");
    }
}

// stop_sending ends Send log: off the air, the radio off (the phone let go
// on the way), core1 and the screen as they were.
static void stop_sending(void) {
    log_line("ble: send log over");
    log_to_send = NULL;
    streaming = false;
    if (stack_up) {
        gap_advertisements_enable(0);
        hci_power_control(HCI_POWER_OFF);
    }
    store_hold_core1(false);
    state_set_status(state, status_before);
    state->wait = wait_before;
}

bool ble_sending(void) {
    return log_to_send != NULL;
}

void ble_poll_radio(uint32_t now_ms) {
    now = now_ms;
    if (stack_up) {
        cyw43_arch_poll();
    }
    if (log_to_send && received_at && now - received_at > RESUME_MS) {
        stop_sending();
    }
}

bool ble_handle_click(void) {
    if (!log_to_send) {
        return false;
    }
    stop_sending();
    return true;
}
