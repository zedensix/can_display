#include "ble.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "btstack.h"
#include "dash.h"
#include "elm327.h"
#include "pico/cyw43_arch.h"
#include "store.h"

// ELM_ADDRESS pins the dongle by MAC address ("AA:BB:CC:DD:EE:FF"). Left
// empty, the first device that looks like an OBD dongle is taken (see
// looks_like_elm). Every candidate is logged.
#define ELM_ADDRESS ""

// Lowercase substrings of advertised names that mark an OBD dongle.
// Vgate's usually advertise as "IOS-Vlink", "Android-Vlink" or
// "vLinker ...".
static const char *const name_hints[] = {"vlink", "vgate", "icar", "obd", "elm"};

// GATT layouts ELM327 BLE dongles use for their serial port:
// notifications carry the dongle's output, writes carry commands. Which
// one depends on the model and firmware. 18F0 is the Vgate iCar Pro 2S's
// (seen on hardware 2026-11-14; it advertises no name, its 18F0 service
// is what matches it), the others are unverified guesses. Every service
// and characteristic is logged: add a line if yours isn't here.
typedef struct {
    uint16_t service, notify, write;
} profile_t;

static const profile_t profiles[] = {
    {0x18F0, 0x2AF0, 0x2AF1},
    {0xFFF0, 0xFFF1, 0xFFF2},
    {0xFFE0, 0xFFE1, 0xFFE1},
};
#define PROFILE_COUNT (int)(sizeof profiles / sizeof profiles[0])

#define SCAN_TIMEOUT_MS 30000
#define STEP_TIMEOUT_MS 10000
#define AUTO_RETRY_MS 5000
#define MAX_SERVICES 16

typedef enum {
    PROMPT,
    STARTING,
    SCANNING,
    CONNECTING,
    DISCOVERING_SERVICES,
    DISCOVERING_CHARACTERISTICS,
    READING_NAME,
    ENABLING_NOTIFICATIONS,
    RUNNING,
    FAILED,
    STANDBY,
    // OFFLINE is a disconnect asked from the settings: like STANDBY, the
    // dongle is let go and the radio off, but the screen stays on.
    OFFLINE,
    // SENDING is Send log: the dongle let go, the dash advertises itself
    // to a phone and sends it the data log.
    SENDING,
} phase_t;

static state_t *state;
static bool auto_connect;
static phase_t phase;
static uint32_t phase_since, now;
static bool stack_up;

static btstack_packet_callback_registration_t hci_registration;
static gatt_client_notification_t notification_listener;
static hci_con_handle_t connection = HCI_CON_HANDLE_INVALID;
static char peer[64];

static gatt_client_service_t services[MAX_SERVICES];
static int service_count, service_index;
static gatt_client_characteristic_t notify_char, write_char, name_char;
static bool have_notify, have_write, have_name;

static void gatt_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void advertise(void);
static void resume(void);
static void start_scan(void);

// last_dongle is the dongle last connected to (from the settings);
// picked is what a click on the prompt connects to, found by a scan in
// the background: the last dongle if it's around, else the first seen.
// connected is the dongle just connected to, for the settings.
static dongle_t last_dongle, picked, connecting_to, connected;
// reconnect_to is the dongle a click on OFFLINE connects back to, once
// the radio is up again.
static dongle_t offline_from, reconnect_to;
static bool picked_last, connected_new, radio_on;
static char picked_name[32];

static elm_t elm;
// disconnecting is set once a reboot is on its way: the disconnection it
// causes isn't a failure, and nothing else should start.
static bool disconnecting;
static char pending[16];
static size_t pending_len;

// The data log on its way to a phone: the part from first on, total
// bytes after a 4-byte header holding total.
#define DATA_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
#define DATA_CCCD_HANDLE ATT_CHARACTERISTIC_c0ff8602_4b1e_4c7a_9e2d_5a3f86d0a001_01_CLIENT_CONFIGURATION_HANDLE
#define CONTROL_HANDLE ATT_CHARACTERISTIC_c0ff8601_4b1e_4c7a_9e2d_5a3f86d0a001_01_VALUE_HANDLE
static datalog_t *send_log;
static hci_con_handle_t phone = HCI_CON_HANDLE_INVALID;
static bool streaming, header_sent;
static uint32_t first, total, sent;
// received_at is when the phone said it got the whole log ('D'): the
// dash goes back to the dongle RESUME_MS later, by itself.
static uint32_t received_at;
#define RESUME_MS 2500

// set_phase moves to p and shows status, or keeps the current one when
// status is NULL.
static void set_phase(phase_t p, const char *status) {
    phase = p;
    phase_since = now;
    if (status) {
        state_set_status(state, status);
    }
}

static void fail(const char *step, const char *error) {
    log_line("fatal: %s: %s", step, error);
    char status[192];
    snprintf(status, sizeof status, "NO LINK\n%s failed:\n%s\n%s", step, error,
             auto_connect ? "retrying..." : "press to retry");
    set_phase(FAILED, status);
    if (stack_up) {
        gap_stop_scan();
    }
}

static const char *name_or_address(const char *name, const bd_addr_t addr) {
    static char out[64];
    snprintf(out, sizeof out, "%s%s%s", name, name[0] ? "\n" : "", bd_addr_to_str(addr));
    return out;
}

static bool looks_like_elm(const bd_addr_t addr, const char *name, uint8_t len, const uint8_t *data) {
    if (ELM_ADDRESS[0]) {
        return strcasecmp(bd_addr_to_str(addr), ELM_ADDRESS) == 0;
    }
    for (int i = 0; i < PROFILE_COUNT; i++) {
        if (ad_data_contains_uuid16(len, data, profiles[i].service)) {
            return true;
        }
    }
    char lower[32];
    size_t n = 0;
    for (; name[n] && n < sizeof lower - 1; n++) {
        lower[n] = (char)tolower((unsigned char)name[n]);
    }
    lower[n] = 0;
    for (size_t i = 0; i < sizeof name_hints / sizeof name_hints[0]; i++) {
        if (strstr(lower, name_hints[i])) {
            return true;
        }
    }
    return false;
}

static void advertised_name(uint8_t len, const uint8_t *data, char name[32]) {
    name[0] = 0;
    ad_context_t it;
    for (ad_iterator_init(&it, len, data); ad_iterator_has_more(&it); ad_iterator_next(&it)) {
        uint8_t type = ad_iterator_get_data_type(&it);
        if (type == BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME || type == BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME) {
            uint8_t n = ad_iterator_get_data_len(&it);
            n = n > 31 ? 31 : n;
            memcpy(name, ad_iterator_get_data(&it), n);
            name[n] = 0;
        }
    }
}

static const char *uuid_str(uint16_t uuid16, const uint8_t uuid128[16]) {
    static char out[40];
    if (uuid16) {
        snprintf(out, sizeof out, "%04x", uuid16);
        return out;
    }
    return uuid128_to_str(uuid128);
}

// Writes go out as "write without response" when the characteristic
// allows it: the dongle answers through notifications anyway. Otherwise
// as acknowledged writes, whose data BTstack reads until they complete,
// hence the separate sent buffer. A command the stack can't take right
// now stays pending for ble_poll.
static void flush_pending(void) {
    static uint8_t sent[sizeof pending];
    if (!pending_len) {
        return;
    }
    memcpy(sent, pending, pending_len);
    uint8_t err;
    if (write_char.properties & ATT_PROPERTY_WRITE_WITHOUT_RESPONSE) {
        err = gatt_client_write_value_of_characteristic_without_response(connection, write_char.value_handle,
                                                                         (uint16_t)pending_len, sent);
    } else {
        err = gatt_client_write_value_of_characteristic(gatt_handler, connection, write_char.value_handle,
                                                        (uint16_t)pending_len, sent);
    }
    if (err == ERROR_CODE_SUCCESS) {
        pending_len = 0;
    }
}

static void elm_write(void *ctx, const char *command) {
    (void)ctx;
    pending_len = strlen(command);
    memcpy(pending, command, pending_len);
    flush_pending();
}

static void elm_log(void *ctx, const char *line) {
    (void)ctx;
    log_line("%s", line);
}

static void elm_ready(void *ctx) {
    (void)ctx;
    set_phase(RUNNING, "");
}

static void elm_failed(void *ctx, const char *step, const char *error) {
    (void)ctx;
    fail(step, error);
}

static void discover_next_service(void) {
    while (service_index < service_count) {
        if (gatt_client_discover_characteristics_for_service(gatt_handler, connection, &services[service_index]) ==
            ERROR_CODE_SUCCESS) {
            return;
        }
        service_index++;
    }
    fail("BLE connect", "no known ELM327 serial service on this device");
}

static void characteristic_found(const gatt_client_characteristic_t *c) {
    log_line("ble:   characteristic %s", uuid_str(c->uuid16, c->uuid128));
    const gatt_client_service_t *s = &services[service_index];
    // The GAP service's device name: dongles that don't advertise a name
    // (the Vgate) still have one here.
    if (s->uuid16 == ORG_BLUETOOTH_SERVICE_GENERIC_ACCESS && c->uuid16 == ORG_BLUETOOTH_CHARACTERISTIC_GAP_DEVICE_NAME) {
        name_char = *c;
        have_name = true;
    }
    for (int i = 0; i < PROFILE_COUNT; i++) {
        if (s->uuid16 != profiles[i].service) {
            continue;
        }
        if (c->uuid16 == profiles[i].notify) {
            notify_char = *c;
            have_notify = true;
        }
        if (c->uuid16 == profiles[i].write) {
            write_char = *c;
            have_write = true;
        }
    }
}

static void enable_notifications(void) {
    gatt_client_listen_for_characteristic_value_updates(&notification_listener, gatt_handler, connection,
                                                        &notify_char);
    gatt_client_write_client_characteristic_configuration(gatt_handler, connection, &notify_char,
                                                          GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
    set_phase(ENABLING_NOTIFICATIONS, NULL);
}

static void query_complete(uint8_t status) {
    switch (phase) {
    case DISCOVERING_SERVICES:
        if (status != ATT_ERROR_SUCCESS || service_count == 0) {
            char error[64];
            snprintf(error, sizeof error, "service discovery failed (ATT 0x%02x, %d services)", status,
                     service_count);
            fail("BLE connect", error);
            return;
        }
        for (int i = 0; i < service_count; i++) {
            log_line("ble: service %s", uuid_str(services[i].uuid16, services[i].uuid128));
        }
        service_index = 0;
        set_phase(DISCOVERING_CHARACTERISTICS, NULL);
        discover_next_service();
        break;
    case DISCOVERING_CHARACTERISTICS:
        if (have_notify && have_write) {
            if (have_name) {
                set_phase(READING_NAME, NULL);
                gatt_client_read_value_of_characteristic(gatt_handler, connection, &name_char);
            } else {
                connected_new = true;
                enable_notifications();
            }
            return;
        }
        have_notify = have_write = false;
        service_index++;
        discover_next_service();
        break;
    case READING_NAME:
        // Without a name, the address will do: not worth failing over.
        log_line("ble: dongle name \"%s\"", connected.name);
        connected_new = true;
        enable_notifications();
        break;
    case ENABLING_NOTIFICATIONS:
        if (status != ATT_ERROR_SUCCESS) {
            char error[48];
            snprintf(error, sizeof error, "notifications refused (ATT 0x%02x)", status);
            fail("BLE connect", error);
            return;
        }
        log_line("elm327: resetting dongle");
        set_phase(RUNNING, "SYNC\ntalking to the ECU");
        elm_start(&elm, state, (elm_callbacks_t){elm_write, elm_log, elm_ready, elm_failed, NULL}, now);
        break;
    default:
        break;
    }
}

static void gatt_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
    case GATT_EVENT_SERVICE_QUERY_RESULT:
        if (service_count < MAX_SERVICES) {
            gatt_event_service_query_result_get_service(packet, &services[service_count++]);
        }
        break;
    case GATT_EVENT_CHARACTERISTIC_QUERY_RESULT: {
        gatt_client_characteristic_t c;
        gatt_event_characteristic_query_result_get_characteristic(packet, &c);
        characteristic_found(&c);
        break;
    }
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT:
        if (phase == READING_NAME) {
            uint16_t len = gatt_event_characteristic_value_query_result_get_value_length(packet);
            len = len < DONGLE_NAME - 1 ? len : DONGLE_NAME - 1;
            memcpy(connected.name, gatt_event_characteristic_value_query_result_get_value(packet), len);
            connected.name[len] = 0;
        }
        break;
    case GATT_EVENT_NOTIFICATION:
        elm_receive(&elm, gatt_event_notification_get_value(packet), gatt_event_notification_get_value_length(packet),
                    now);
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        query_complete(gatt_event_query_complete_get_att_status(packet));
        break;
    default:
        break;
    }
}

// connect_to stops scanning and connects to a dongle.
static void connect_to(const bd_addr_t addr, uint8_t type, const char *name) {
    gap_stop_scan();
    snprintf(peer, sizeof peer, "%s", name_or_address(name, addr));
    char status[192];
    snprintf(status, sizeof status, "LINKING\n%s", peer);
    set_phase(CONNECTING, status);
    log_line("ble: connecting to %s", bd_addr_to_str(addr));
    connecting_to = (dongle_t){.set = true, .type = type};
    memcpy(connecting_to.addr, addr, sizeof connecting_to.addr);
    snprintf(connecting_to.name, sizeof connecting_to.name, "%s", name);
    uint8_t err = gap_connect(addr, (bd_addr_type_t)type);
    if (err != ERROR_CODE_SUCCESS) {
        char error[32];
        snprintf(error, sizeof error, "gap_connect error 0x%02x", err);
        fail("BLE connect", error);
    }
}

// prompt_report is a device seen while the prompt waits for a click: a
// dongle becomes what the click connects to, the last one used first. It
// shows by name: the one it advertises, else the one read from it last
// time; by address only without either.
static void prompt_report(const bd_addr_t addr, uint8_t type, const char *name, uint8_t len, const uint8_t *data) {
    if (!looks_like_elm(addr, name, len, data)) {
        return;
    }
    bool last = last_dongle.set && memcmp(last_dongle.addr, addr, sizeof last_dongle.addr) == 0;
    bool same = picked.set && memcmp(picked.addr, addr, sizeof picked.addr) == 0;
    // A name arriving late (in a scan response) for the dongle picked.
    bool named = same && name[0] && strcmp(picked_name, name) != 0;
    if (picked.set && !named && (picked_last || !last)) {
        return;
    }
    if (!same) {
        picked = (dongle_t){.set = true, .type = type};
        memcpy(picked.addr, addr, sizeof picked.addr);
        picked_last = last;
        picked_name[0] = 0;
    }
    if (name[0]) {
        snprintf(picked_name, sizeof picked_name, "%s", name);
    } else if (last && !picked_name[0]) {
        snprintf(picked_name, sizeof picked_name, "%s", last_dongle.name);
    }
    log_line("ble: prompt offers %s %s%s", bd_addr_to_str(addr), picked_name, last ? " (last used)" : "");
    char status[192];
    snprintf(status, sizeof status, "READY\npress to connect to\n%s%s", picked_name[0] ? picked_name : bd_addr_to_str(addr),
             last ? "\n(last used)" : "");
    state_set_status(state, status);
}

static void advertising_report(uint8_t *packet) {
    if (phase != SCANNING && phase != PROMPT) {
        return;
    }
    bd_addr_t addr;
    gap_event_advertising_report_get_address(packet, addr);
    uint8_t type = gap_event_advertising_report_get_address_type(packet);
    uint8_t len = gap_event_advertising_report_get_data_length(packet);
    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    char name[32];
    advertised_name(len, data, name);
    if (phase == PROMPT) {
        prompt_report(addr, type, name, len, data);
        return;
    }
    if (!looks_like_elm(addr, name, len, data)) {
        char status[192];
        snprintf(status, sizeof status, "SEARCHING\nlast ignored:\n%s", name_or_address(name, addr));
        state_set_status(state, status);
        return;
    }
    log_line("ble: candidate %s %s RSSI %d", bd_addr_to_str(addr), name,
             (int8_t)gap_event_advertising_report_get_rssi(packet));
    connect_to(addr, type, name);
}

static void connection_complete(uint8_t *packet) {
    if (phase != CONNECTING) {
        return;
    }
    uint8_t status = gap_subevent_le_connection_complete_get_status(packet);
    if (status != ERROR_CODE_SUCCESS) {
        char error[40];
        snprintf(error, sizeof error, "connection failed (HCI 0x%02x)", status);
        fail("BLE connect", error);
        return;
    }
    connection = gap_subevent_le_connection_complete_get_connection_handle(packet);
    // Handed to the settings once its name is read (or known missing).
    connected = connecting_to;
    have_notify = have_write = have_name = false;
    char text[192];
    snprintf(text, sizeof text, "HANDSHAKE\n%s", peer);
    set_phase(DISCOVERING_SERVICES, text);
    service_count = 0;
    gatt_client_discover_primary_services(gatt_handler, connection);
}

static void hci_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type != HCI_EVENT_PACKET) {
        return;
    }
    switch (hci_event_packet_get_type(packet)) {
    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) {
            break;
        }
        radio_on = true;
        if (send_log) {
            advertise();
        } else if (reconnect_to.set) {
            dongle_t d = reconnect_to;
            reconnect_to.set = false;
            connect_to(d.addr, d.type, d.name);
        } else if (phase == PROMPT) {
            // In the background, for the prompt to name a dongle.
            gap_set_scan_parameters(1, 0x30, 0x30);
            gap_start_scan();
        } else if (phase == STARTING) {
            start_scan();
        }
        break;
    case GAP_EVENT_ADVERTISING_REPORT:
        advertising_report(packet);
        break;
    case HCI_EVENT_META_GAP:
        // Covers both the legacy and the enhanced (BT 5) connection events.
        if (hci_event_gap_meta_get_subevent_code(packet) != GAP_SUBEVENT_LE_CONNECTION_COMPLETE) {
            break;
        }
        if (phase == SENDING) {
            if (gap_subevent_le_connection_complete_get_status(packet) == ERROR_CODE_SUCCESS) {
                phone = gap_subevent_le_connection_complete_get_connection_handle(packet);
                log_line("ble: phone connected");
                state_set_status(state, "SEND LOG\nphone connected");
            }
        } else {
            connection_complete(packet);
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) == phone) {
            // The phone gone: still sending, back on the air for it.
            phone = HCI_CON_HANDLE_INVALID;
            streaming = false;
            if (phase == SENDING && !received_at) {
                advertise();
            }
            break;
        }
        connection = HCI_CON_HANDLE_INVALID;
        if (phase == SENDING) {
            // The dongle let go: on the air for the phone.
            advertise();
        } else if (phase == STANDBY || phase == OFFLINE) {
            hci_power_control(HCI_POWER_OFF);
        } else if (phase != FAILED && !disconnecting) {
            char error[40];
            snprintf(error, sizeof error, "disconnected (HCI 0x%02x)",
                     hci_event_disconnection_complete_get_reason(packet));
            fail("BLE", error);
        }
        break;
    default:
        break;
    }
}

static void advertise(void) {
    // Flags, then the service's UUID (bytes reversed) for the phone to
    // filter on; the name goes in the scan response.
    static uint8_t adv[] = {0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06, 0x11,
                            BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, 0x01, 0xa0, 0xd0, 0x86,
                            0x3f, 0x5a, 0x2d, 0x9e, 0x7a, 0x4c, 0x1e, 0x4b, 0x00, 0x86, 0xff, 0xc0};
    static uint8_t scan_response[] = {0x0A, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'G', 'T', '8', '6', ' ',
                                      'D', 'A', 'S', 'H'};
    bd_addr_t anyone = {0};
    gap_advertisements_set_params(0x0030, 0x0030, 0, 0, anyone, 0x07, 0x00);
    gap_advertisements_set_data(sizeof adv, adv);
    gap_scan_response_set_data(sizeof scan_response, scan_response);
    gap_advertisements_enable(1);
    set_phase(SENDING, "SEND LOG\nopen the page\non your phone\npress: back");
}

static void send_chunk(void) {
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
        datalog_read(send_log, first + sent, chunk, n);
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

static int att_write(hci_con_handle_t con, uint16_t handle, uint16_t mode, uint16_t offset, uint8_t *buffer,
                     uint16_t size) {
    (void)mode;
    (void)offset;
    if (handle == DATA_CCCD_HANDLE) {
        return 0;
    }
    if (handle == CONTROL_HANDLE && size >= 1 && buffer[0] == 'D' && send_log) {
        log_line("ble: the phone got the data log");
        state_set_status(state, "SEND LOG\ndownloaded\nback to the gauges");
        received_at = now ? now : 1;
        return 0;
    }
    if (handle == CONTROL_HANDLE && size >= 1 && buffer[0] == 'S' && send_log) {
        first = datalog_oldest(send_log);
        total = send_log->dev.size - first;
        sent = 0;
        header_sent = false;
        streaming = true;
        log_line("ble: sending the data log, %lu bytes", (unsigned long)total);
        att_server_request_can_send_now_event(con);
    }
    return 0;
}

static void att_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    (void)channel;
    (void)size;
    if (packet_type == HCI_EVENT_PACKET && hci_event_packet_get_type(packet) == ATT_EVENT_CAN_SEND_NOW) {
        send_chunk();
    }
}

static void start_scan(void) {
    log_line("ble: scanning for the OBD dongle (a phone connected to it hides it)");
    set_phase(SCANNING, "SEARCHING\nlooking for the OBD dongle");
    gap_set_scan_parameters(1, 0x30, 0x30); // active: also get the scan responses, with names
    gap_start_scan();
}

// resume ends Send log: off the air, the phone let go, back to the dongle
// without a reboot.
static void resume(void) {
    log_line("ble: send log over, back to the dongle");
    send_log = NULL;
    streaming = false;
    received_at = 0;
    gap_advertisements_enable(0);
    if (phone != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(phone);
    }
    start_scan();
}

// start_radio brings the radio and BTstack up; the HCI working event
// carries on from the phase it finds.
static void start_radio(void) {
    if (cyw43_arch_init()) {
        fail("Bluetooth", "cyw43 init failed");
        return;
    }
    stack_up = true;
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gatt_client_init();
    att_server_init(profile_data, NULL, att_write);
    att_server_register_packet_handler(att_handler);
    hci_registration.callback = hci_handler;
    hci_add_event_handler(&hci_registration);
    hci_power_control(HCI_POWER_ON);
}

static void start(void) {
    set_phase(STARTING, "IGNITION\nstarting Bluetooth");
    start_radio();
}

void ble_init(state_t *s, bool auto_start, const dongle_t *last) {
    state = s;
    auto_connect = auto_start;
    last_dongle = *last;
    if (auto_start) {
        start();
    } else {
        set_phase(PROMPT, "READY\nlooking for dongles\npress to connect");
        start_radio();
    }
}

bool ble_take_dongle(dongle_t *out) {
    if (!connected_new) {
        return false;
    }
    connected_new = false;
    *out = connected;
    return true;
}

static void check_timeouts(void) {
    if (disconnecting) {
        return;
    }
    uint32_t elapsed = now - phase_since;
    switch (phase) {
    case STARTING:
        if (elapsed > STEP_TIMEOUT_MS) {
            fail("Bluetooth", "the radio didn't start");
        }
        break;
    case SCANNING:
        if (elapsed > SCAN_TIMEOUT_MS) {
            fail("BLE scan", "no OBD dongle found");
        }
        break;
    case CONNECTING:
        if (elapsed > STEP_TIMEOUT_MS) {
            gap_connect_cancel();
            fail("BLE connect", "the dongle didn't answer");
        }
        break;
    case DISCOVERING_SERVICES:
    case DISCOVERING_CHARACTERISTICS:
    case READING_NAME:
    case ENABLING_NOTIFICATIONS:
        if (elapsed > STEP_TIMEOUT_MS) {
            fail("BLE connect", "GATT discovery timed out");
        }
        break;
    case SENDING:
        if (received_at && now - received_at > RESUME_MS) {
            resume();
        }
        break;
    case FAILED:
        if (auto_connect && elapsed > AUTO_RETRY_MS) {
            reboot_to_connect();
        }
        break;
    default:
        break;
    }
}

static void standby(void) {
    log_line("car off: the engine ECU was silent for %ds, standby", CAR_OFF_MS / 1000);
    set_phase(STANDBY, "SLEEP\ncar off, press to wake");
    if (connection != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(connection);
    } else {
        hci_power_control(HCI_POWER_OFF);
    }
}

void ble_poll(uint32_t now_ms) {
    now = now_ms;
    if (stack_up) {
        cyw43_arch_poll();
    }
    if (phase == RUNNING && elm.phase != ELM_OFF) {
        flush_pending();
        elm_tick(&elm, now_ms);
        if (elm.phase == ELM_POLL && now_ms - elm.ecu_ms > CAR_OFF_MS) {
            standby();
        }
    }
    check_timeouts();
}

void ble_go_offline(void) {
    if (!stack_up || phase == STANDBY || phase == OFFLINE) {
        return;
    }
    log_line("ble: disconnected from the settings");
    phase_t was = phase;
    // The dongle a click connects back to: this session's, else the last.
    offline_from = connected.set ? connected : last_dongle;
    char status[96];
    if (offline_from.set) {
        snprintf(status, sizeof status, "OFFLINE\npress to connect to\n%s",
                 offline_from.name[0] ? offline_from.name : bd_addr_to_str(offline_from.addr));
    } else {
        snprintf(status, sizeof status, "OFFLINE\npress to connect");
    }
    set_phase(OFFLINE, status);
    if (was == SCANNING) {
        gap_stop_scan();
    } else if (was == CONNECTING) {
        gap_connect_cancel();
    }
    if (connection != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(connection);
    } else {
        hci_power_control(HCI_POWER_OFF);
    }
}

void ble_send_log(datalog_t *log) {
    if (!log || phase == SENDING) {
        return;
    }
    send_log = log;
    log_line("ble: send log");
    if (!stack_up) {
        start();
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
        advertise();
    }
}

void ble_scan_ram(void) {
    if (phase == RUNNING) {
        elm_scan_ram(&elm);
    }
}

void ble_listen_can(void) {
    if (phase == RUNNING) {
        elm_listen_can(&elm);
    }
}

bool ble_standby(void) {
    return phase == STANDBY;
}

bool ble_prompt_click(uint32_t now_ms) {
    now = now_ms;
    switch (phase) {
    case PROMPT:
        if (picked.set) {
            connect_to(picked.addr, picked.type, picked_name);
        } else if (radio_on) {
            gap_stop_scan();
            start_scan();
        } else {
            // The radio is still starting: it scans once up.
            set_phase(STARTING, "IGNITION\nstarting Bluetooth");
        }
        return true;
    case FAILED:
        if (!auto_connect) {
            reboot_to_connect();
        }
        return true;
    case OFFLINE:
        if (!offline_from.set) {
            reboot_to_connect();
            return true;
        }
        // Straight back to it, no reboot: the radio comes up, then connects.
        reconnect_to = offline_from;
        set_phase(STARTING, "IGNITION\nstarting Bluetooth");
        hci_power_control(HCI_POWER_ON);
        return true;
    case STANDBY:
        reboot_to_connect();
        return true;
    case SENDING:
        resume();
        return true;
    default:
        return false;
    }
}

void ble_disconnect(void) {
    disconnecting = true;
    if (connection != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(connection);
    }
}
