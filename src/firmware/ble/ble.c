#include <stdio.h>
#include <string.h>

#include "ble_internal.h"
#include "hardware/store.h"
#include "pico/cyw43_arch.h"

#define SCAN_TIMEOUT_MS 30000
#define STEP_TIMEOUT_MS 10000
#define AUTO_RETRY_MS 5000

state_t *state;
bool auto_connect;
phase_t phase;
uint32_t phase_since, now;
bool stack_up;

static btstack_packet_callback_registration_t hci_registration;
hci_con_handle_t connection = HCI_CON_HANDLE_INVALID;
char peer[64];

dongle_t last_dongle, picked, connecting_to, connected;
dongle_t reconnect_to;
bool connected_new, radio_on;
char picked_name[32];

elm_t elm;
bool disconnecting;

// set_phase moves to p and shows status, or keeps the current one when
// status is NULL.
void set_phase(phase_t p, const char *status) {
    phase = p;
    phase_since = now;
    state->wait = p == PROMPT || p == OFFLINE ? WAIT_CHOICE : p == FAILED && !auto_connect ? WAIT_CLICK : WAIT_NONE;
    if (status) {
        state_set_status(state, status);
    }
}

void fail_connection(const char *step, const char *error) {
    log_line("fatal: %s: %s", step, error);
    char status[192];
    snprintf(status, sizeof status, "NO LINK\n%s failed:\n%s\n%s", step, error,
             auto_connect ? "retrying..." : "press to retry");
    set_phase(FAILED, status);
    if (stack_up) {
        gap_stop_scan();
    }
}

static void handle_connection_complete(uint8_t *packet) {
    if (phase != CONNECTING) {
        return;
    }
    uint8_t status = gap_subevent_le_connection_complete_get_status(packet);
    if (status != ERROR_CODE_SUCCESS) {
        char error[40];
        snprintf(error, sizeof error, "connection failed (HCI 0x%02x)", status);
        fail_connection("BLE connect", error);
        return;
    }
    connection = gap_subevent_le_connection_complete_get_connection_handle(packet);
    // Handed to the settings once its name is read (or known missing).
    connected = connecting_to;
    char text[192];
    snprintf(text, sizeof text, "HANDSHAKE\n%s", peer);
    set_phase(DISCOVERING_SERVICES, text);
    discover_dongle_services();
}

static void handle_hci_event(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
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
        if (log_to_send) {
            advertise_to_phone();
        } else if (reconnect_to.set) {
            dongle_t d = reconnect_to;
            reconnect_to.set = false;
            connect_to_dongle(d.addr, d.type, d.name);
        } else if (phase == PROMPT) {
            // In the background, for the prompt to name a dongle.
            gap_set_scan_parameters(1, 0x30, 0x30);
            gap_start_scan();
        } else if (phase == STARTING) {
            start_dongle_scan();
        }
        break;
    case GAP_EVENT_ADVERTISING_REPORT:
        handle_advertising_report(packet);
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
            handle_connection_complete(packet);
        }
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        if (hci_event_disconnection_complete_get_connection_handle(packet) == phone) {
            // The phone gone: still sending, back on the air for it.
            phone = HCI_CON_HANDLE_INVALID;
            streaming = false;
            if (phase == SENDING && !received_at) {
                advertise_to_phone();
            }
            break;
        }
        connection = HCI_CON_HANDLE_INVALID;
        if (phase == SENDING) {
            // The dongle let go: on the air for the phone.
            advertise_to_phone();
        } else if (phase == STANDBY || phase == OFFLINE) {
            hci_power_control(HCI_POWER_OFF);
        } else if (phase != FAILED && !disconnecting) {
            char error[40];
            snprintf(error, sizeof error, "disconnected (HCI 0x%02x)",
                     hci_event_disconnection_complete_get_reason(packet));
            fail_connection("BLE", error);
        }
        break;
    default:
        break;
    }
}

// start_radio brings the radio and BTstack up; the HCI working event
// carries on from the phase it finds.
static void start_radio(void) {
    if (cyw43_arch_init()) {
        fail_connection("Bluetooth", "cyw43 init failed");
        return;
    }
    stack_up = true;
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    gatt_client_init();
    start_att_server();
    hci_registration.callback = handle_hci_event;
    hci_add_event_handler(&hci_registration);
    hci_power_control(HCI_POWER_ON);
}

void start_bluetooth(void) {
    set_phase(STARTING, "IGNITION\nstarting Bluetooth");
    start_radio();
}

void ble_init(state_t *s, bool auto_start, const dongle_t *last) {
    state = s;
    auto_connect = auto_start;
    last_dongle = *last;
    if (auto_start) {
        start_bluetooth();
    } else {
        set_phase(PROMPT, "READY\nlooking for dongles");
        start_radio();
    }
}

bool ble_take_connected_dongle(dongle_t *out) {
    if (!connected_new) {
        return false;
    }
    connected_new = false;
    *out = connected;
    return true;
}

static void check_phase_timeouts(void) {
    if (disconnecting) {
        return;
    }
    uint32_t elapsed = now - phase_since;
    switch (phase) {
    case STARTING:
        if (elapsed > STEP_TIMEOUT_MS) {
            fail_connection("Bluetooth", "the radio didn't start");
        }
        break;
    case SCANNING:
        if (elapsed > SCAN_TIMEOUT_MS) {
            fail_connection("BLE scan", "no OBD dongle found");
        }
        break;
    case CONNECTING:
        if (elapsed > STEP_TIMEOUT_MS) {
            gap_connect_cancel();
            fail_connection("BLE connect", "the dongle didn't answer");
        }
        break;
    case DISCOVERING_SERVICES:
    case DISCOVERING_CHARACTERISTICS:
    case READING_NAME:
    case ENABLING_NOTIFICATIONS:
        if (elapsed > STEP_TIMEOUT_MS) {
            fail_connection("BLE connect", "GATT discovery timed out");
        }
        break;
    case SENDING:
        if (received_at && now - received_at > RESUME_MS) {
            back_to_dongle();
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

static void enter_standby(void) {
    log_line("car off: the engine ECU was silent for %ds, standby", CAR_OFF_MS / 1000);
    set_phase(STANDBY, "SLEEP\ncar off, press to wake");
    if (connection != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(connection);
    } else {
        hci_power_control(HCI_POWER_OFF);
    }
}

void ble_poll_radio(uint32_t now_ms) {
    now = now_ms;
    if (stack_up) {
        cyw43_arch_poll();
    }
    if (phase == RUNNING && elm.phase != ELM_OFF) {
        write_pending_command();
        elm_check_timers(&elm, now_ms);
        if (elm.phase == ELM_POLL && now_ms - elm.ecu_ms > CAR_OFF_MS) {
            enter_standby();
        }
    }
    check_phase_timeouts();
}
