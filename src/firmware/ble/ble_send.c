#include <stdio.h>

#include "ble_internal.h"
#include "dash.h"
#include "hardware/store.h"

// The data log on its way to a phone: the part from first on, total
// bytes after a 4-byte header holding total.
#define DATA_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
#define DATA_CCCD_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_CLIENT_CONFIGURATION_HANDLE
#define CONTROL_HANDLE ATT_CHARACTERISTIC_c0ff8601_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
datalog_t *log_to_send;
hci_con_handle_t phone = HCI_CON_HANDLE_INVALID;
bool streaming;
static bool header_sent;
static uint32_t first, total, sent;
uint32_t received_at;

void advertise_to_phone(void) {
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
    set_phase(SENDING, "SEND LOG\nopen the page\non your phone\npress: back");
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
    if (handle == DATA_CCCD_HANDLE) {
        return 0;
    }
    if (handle == CONTROL_HANDLE && size >= 1 && buffer[0] == 'D' && log_to_send) {
        log_line("ble: the phone got the data log");
        state_set_status(state, "SEND LOG\ndownloaded\nback to the gauges");
        received_at = now ? now : 1;
        return 0;
    }
    if (handle == CONTROL_HANDLE && size >= 1 && buffer[0] == 'S' && log_to_send) {
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

// back_to_dongle ends Send log: off the air, the phone let go, back to the dongle
// without a reboot.
void back_to_dongle(void) {
    log_line("ble: send log over, back to the dongle");
    log_to_send = NULL;
    streaming = false;
    received_at = 0;
    gap_advertisements_enable(0);
    if (phone != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(phone);
    }
    start_dongle_scan();
}

void ble_send_log(datalog_t *log) {
    if (!log || phase == SENDING) {
        return;
    }
    log_to_send = log;
    log_line("ble: send log");
    if (!stack_up) {
        start_bluetooth();
        return;
    }
    switch (phase) {
    case STANDBY:
    case OFFLINE:
        hci_power_control(HCI_POWER_ON);
        set_phase(SENDING, "SEND LOG\nstarting Bluetooth");
        return;
    case SCANNING:
        gap_stop_scan();
        break;
    case CONNECTING:
        gap_connect_cancel();
        break;
    default:
        break;
    }
    if (connection != HCI_CON_HANDLE_INVALID) {
        set_phase(SENDING, "SEND LOG\nletting the dongle go");
        gap_disconnect(connection);
    } else {
        advertise_to_phone();
    }
}

// start_att_server serves the dash's GATT profile (dash.gatt), which the
// phone reads the log from.
void start_att_server(void) {
    att_server_init(profile_data, NULL, handle_att_write);
    att_server_register_packet_handler(handle_att_event);
}
