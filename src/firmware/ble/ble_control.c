#include <stdio.h>

#include "ble_internal.h"
#include "hardware/store.h"

// offline_from is the dongle a click on OFFLINE connects back to.
static dongle_t offline_from;

void ble_dongle_asleep(void) {
    log_line("ble: the dongle let go with the engine ECU silent: car off, the dongle asleep");
    offline_from = connected.set ? connected : last_dongle;
    set_phase(OFFLINE, "DISCONNECTED\npress to reconnect");
    hci_power_control(HCI_POWER_OFF);
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
        snprintf(status, sizeof status, "OFFLINE\n%s",
                 offline_from.name[0] ? offline_from.name : bd_addr_to_str(offline_from.addr));
    } else {
        snprintf(status, sizeof status, "OFFLINE");
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

void ble_request_ram_scan(void) {
    if (phase == RUNNING) {
        elm_request_ram_scan(&elm);
    }
}

bool ble_in_standby(void) {
    return phase == STANDBY;
}

void ble_request_can_listen(void) {
    if (phase == RUNNING) {
        elm_request_can_listen(&elm);
    }
}

bool ble_handle_click(uint32_t now_ms, choice_t choice) {
    now = now_ms;
    switch (phase) {
    case PROMPT:
        if (picked.set) {
            connect_to_dongle(picked.addr, picked.type, picked_name);
        } else if (radio_on) {
            gap_stop_scan();
            start_dongle_scan();
        } else {
            // The radio is still starting: it scans once up.
            set_phase(STARTING, "IGNITION\nstarting Bluetooth");
        }
        return true;
    case STARTING:
    case SCANNING:
    case CONNECTING:
    case DISCOVERING_SERVICES:
    case DISCOVERING_CHARACTERISTICS:
    case READING_NAME:
    case ENABLING_NOTIFICATIONS:
    case FAILED:
        // Retry: a clean start, connecting right away.
        reboot_to_connect();
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
        back_to_dongle();
        return true;
    case RUNNING:
        // A listen's prompt: Next, or Cancel in Settings' place. Else a
        // RAM scan's or a listen's Cancel.
        if (choice == CHOICE_SETTINGS) {
            return elm_cancel_listen(&elm, now_ms);
        }
        return elm_listen_click(&elm, now_ms) || elm_cancel_ram_scan(&elm) || elm_cancel_listen(&elm, now_ms);
    default:
        return false;
    }
}

void ble_disconnect_before_reboot(void) {
    disconnecting = true;
    if (connection != HCI_CON_HANDLE_INVALID) {
        gap_disconnect(connection);
    }
}
