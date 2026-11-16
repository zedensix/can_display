#include <stdio.h>
#include <string.h>

#include "ble_internal.h"
#include "hardware/store.h"

// GATT layouts ELM327 BLE dongles use for their serial port:
// notifications carry the dongle's output, writes carry commands. Which
// one depends on the model and firmware. 18F0 is the Vgate iCar Pro 2S's
// (seen on hardware 2026-11-14; it advertises no name, its 18F0 service
// is what matches it), the others are unverified guesses. Every service
// and characteristic is logged: add a line if yours isn't here.
const profile_t profiles[] = {
    {0x18F0, 0x2AF0, 0x2AF1},
    {0xFFF0, 0xFFF1, 0xFFF2},
    {0xFFE0, 0xFFE1, 0xFFE1},
};
const int profile_count = sizeof profiles / sizeof profiles[0];

#define MAX_SERVICES 16

static gatt_client_notification_t notification_listener;
static gatt_client_service_t services[MAX_SERVICES];
static int service_count, service_index;
static gatt_client_characteristic_t notify_char, write_char, name_char;
static bool have_notify, have_write, have_name;
static char pending[16];
static size_t pending_len;

static void handle_gatt_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static const char *uuid_to_str(uint16_t uuid16, const uint8_t uuid128[16]) {
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
// now stays pending for ble_poll_radio.
void write_pending_command(void) {
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
        err = gatt_client_write_value_of_characteristic(handle_gatt_event, connection, write_char.value_handle,
                                                        (uint16_t)pending_len, sent);
    }
    if (err == ERROR_CODE_SUCCESS) {
        pending_len = 0;
    }
}

static void write_to_dongle(void *ctx, const char *command) {
    (void)ctx;
    pending_len = strlen(command);
    memcpy(pending, command, pending_len);
    write_pending_command();
}

static void log_from_elm(void *ctx, const char *line) {
    (void)ctx;
    log_line("%s", line);
}

static void on_elm_ready(void *ctx) {
    (void)ctx;
    set_phase(RUNNING, "");
}

static void on_elm_failed(void *ctx, const char *step, const char *error) {
    (void)ctx;
    fail_connection(step, error);
}

static void discover_next_service_characteristics(void) {
    while (service_index < service_count) {
        if (gatt_client_discover_characteristics_for_service(handle_gatt_event, connection, &services[service_index]) ==
            ERROR_CODE_SUCCESS) {
            return;
        }
        service_index++;
    }
    fail_connection("BLE connect", "no known ELM327 serial service on this device");
}

static void note_characteristic(const gatt_client_characteristic_t *c) {
    log_line("ble:   characteristic %s", uuid_to_str(c->uuid16, c->uuid128));
    const gatt_client_service_t *s = &services[service_index];
    // The GAP service's device name: dongles that don't advertise a name
    // (the Vgate) still have one here.
    if (s->uuid16 == ORG_BLUETOOTH_SERVICE_GENERIC_ACCESS &&
        c->uuid16 == ORG_BLUETOOTH_CHARACTERISTIC_GAP_DEVICE_NAME) {
        name_char = *c;
        have_name = true;
    }
    for (int i = 0; i < profile_count; i++) {
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

static void enable_dongle_notifications(void) {
    gatt_client_listen_for_characteristic_value_updates(&notification_listener, handle_gatt_event, connection,
                                                        &notify_char);
    gatt_client_write_client_characteristic_configuration(handle_gatt_event, connection, &notify_char,
                                                          GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION);
    set_phase(ENABLING_NOTIFICATIONS, NULL);
}

static void handle_gatt_query_complete(uint8_t status) {
    switch (phase) {
    case DISCOVERING_SERVICES:
        if (status != ATT_ERROR_SUCCESS || service_count == 0) {
            char error[64];
            snprintf(error, sizeof error, "service discovery failed (ATT 0x%02x, %d services)", status, service_count);
            fail_connection("BLE connect", error);
            return;
        }
        for (int i = 0; i < service_count; i++) {
            log_line("ble: service %s", uuid_to_str(services[i].uuid16, services[i].uuid128));
        }
        service_index = 0;
        set_phase(DISCOVERING_CHARACTERISTICS, NULL);
        discover_next_service_characteristics();
        break;
    case DISCOVERING_CHARACTERISTICS:
        if (have_notify && have_write) {
            if (have_name) {
                set_phase(READING_NAME, NULL);
                gatt_client_read_value_of_characteristic(handle_gatt_event, connection, &name_char);
            } else {
                connected_new = true;
                enable_dongle_notifications();
            }
            return;
        }
        have_notify = have_write = false;
        service_index++;
        discover_next_service_characteristics();
        break;
    case READING_NAME:
        // Without a name, the address will do: not worth failing over.
        log_line("ble: dongle name \"%s\"", connected.name);
        connected_new = true;
        enable_dongle_notifications();
        break;
    case ENABLING_NOTIFICATIONS:
        if (status != ATT_ERROR_SUCCESS) {
            char error[48];
            snprintf(error, sizeof error, "notifications refused (ATT 0x%02x)", status);
            fail_connection("BLE connect", error);
            return;
        }
        log_line("elm327: resetting dongle");
        set_phase(RUNNING, "SYNC\ntalking to the ECU");
        elm_start_session(&elm, state,
                          (elm_callbacks_t){write_to_dongle, log_from_elm, on_elm_ready, on_elm_failed, NULL}, now);
        break;
    default:
        break;
    }
}

static void handle_gatt_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
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
        note_characteristic(&c);
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
        elm_feed_dongle_output(&elm, gatt_event_notification_get_value(packet),
                               gatt_event_notification_get_value_length(packet), now);
        break;
    case GATT_EVENT_QUERY_COMPLETE:
        handle_gatt_query_complete(gatt_event_query_complete_get_att_status(packet));
        break;
    default:
        break;
    }
}

// discover_dongle_services starts the GATT discovery on a new connection:
// its services, then their characteristics, for the serial port's.
void discover_dongle_services(void) {
    have_notify = have_write = have_name = false;
    service_count = 0;
    gatt_client_discover_primary_services(handle_gatt_event, connection);
}
