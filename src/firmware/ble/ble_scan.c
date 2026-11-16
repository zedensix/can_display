#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ble_internal.h"
#include "hardware/store.h"

// ELM_ADDRESS pins the dongle by MAC address ("AA:BB:CC:DD:EE:FF"). Left
// empty, the first device that looks like an OBD dongle is taken (see
// looks_like_obd_dongle). Every candidate is logged.
#define ELM_ADDRESS ""

// Lowercase substrings of advertised names that mark an OBD dongle.
// Vgate's usually advertise as "IOS-Vlink", "Android-Vlink" or
// "vLinker ...".
static const char *const name_hints[] = {"vlink", "vgate", "icar", "obd", "elm"};

static bool picked_last;

static const char *name_or_address(const char *name, const bd_addr_t addr) {
    static char out[64];
    snprintf(out, sizeof out, "%s%s%s", name, name[0] ? "\n" : "", bd_addr_to_str(addr));
    return out;
}

static bool looks_like_obd_dongle(const bd_addr_t addr, const char *name, uint8_t len, const uint8_t *data) {
    if (ELM_ADDRESS[0]) {
        return strcasecmp(bd_addr_to_str(addr), ELM_ADDRESS) == 0;
    }
    for (int i = 0; i < profile_count; i++) {
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

static void read_advertised_name(uint8_t len, const uint8_t *data, char name[32]) {
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

// connect_to_dongle stops scanning and connects to a dongle.
void connect_to_dongle(const bd_addr_t addr, uint8_t type, const char *name) {
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
        fail_connection("BLE connect", error);
    }
}

// offer_dongle_on_prompt is a device seen while the prompt waits for a click: a
// dongle becomes what the click connects to, the last one used first. It
// shows by name: the one it advertises, else the one read from it last
// time; by address only without either.
static void offer_dongle_on_prompt(const bd_addr_t addr, uint8_t type, const char *name, uint8_t len,
                                   const uint8_t *data) {
    if (!looks_like_obd_dongle(addr, name, len, data)) {
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
    snprintf(status, sizeof status, "READY\n%s%s", picked_name[0] ? picked_name : bd_addr_to_str(addr),
             last ? "\n(last used)" : "");
    state_set_status(state, status);
}

void handle_advertising_report(uint8_t *packet) {
    if (phase != SCANNING && phase != PROMPT) {
        return;
    }
    bd_addr_t addr;
    gap_event_advertising_report_get_address(packet, addr);
    uint8_t type = gap_event_advertising_report_get_address_type(packet);
    uint8_t len = gap_event_advertising_report_get_data_length(packet);
    const uint8_t *data = gap_event_advertising_report_get_data(packet);
    char name[32];
    read_advertised_name(len, data, name);
    if (phase == PROMPT) {
        offer_dongle_on_prompt(addr, type, name, len, data);
        return;
    }
    if (!looks_like_obd_dongle(addr, name, len, data)) {
        char status[192];
        snprintf(status, sizeof status, "SEARCHING\nlast ignored:\n%s", name_or_address(name, addr));
        state_set_status(state, status);
        return;
    }
    log_line("ble: candidate %s %s RSSI %d", bd_addr_to_str(addr), name,
             (int8_t)gap_event_advertising_report_get_rssi(packet));
    connect_to_dongle(addr, type, name);
}

void start_dongle_scan(void) {
    log_line("ble: scanning for the OBD dongle (a phone connected to it hides it)");
    set_phase(SCANNING, "SEARCHING\nlooking for the OBD dongle");
    gap_set_scan_parameters(1, 0x30, 0x30); // active: also get the scan responses, with names
    gap_start_scan();
}
