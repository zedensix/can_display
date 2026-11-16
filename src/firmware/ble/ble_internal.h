#pragma once

// What the BLE data source's files share. ble.c brings the radio up,
// runs the phases and the connection; ble_scan.c finds the dongle;
// ble_gatt.c finds its serial port and runs the ELM327 session over it;
// ble_send.c sends the data log to a phone; ble_control.c is what the
// rest of the dash asks for (ble.h).

#include "ble.h"
#include "btstack.h"
#include "obd/elm327.h"

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

// GATT layouts of ELM327 BLE dongles' serial ports (see ble_gatt.c).
typedef struct {
    uint16_t service, notify, write;
} profile_t;
extern const profile_t profiles[];
extern const int profile_count;

// received_at (ble_send.c) is when the phone said it got the whole log
// ('D'): the dash goes back to the dongle RESUME_MS later, by itself.
#define RESUME_MS 2500

extern state_t *state;
extern bool auto_connect;
extern phase_t phase;
extern uint32_t phase_since, now;
extern bool stack_up;
extern hci_con_handle_t connection;
extern char peer[64];

// last_dongle is the dongle last connected to (from the settings);
// picked is what a click on the prompt connects to, found by a scan in
// the background: the last dongle if it's around, else the first seen.
// connected is the dongle just connected to, for the settings.
extern dongle_t last_dongle, picked, connecting_to, connected;
// reconnect_to is the dongle a click on OFFLINE connects back to, once
// the radio is up again.
extern dongle_t reconnect_to;
extern bool connected_new, radio_on;
extern char picked_name[32];

extern elm_t elm;
// disconnecting is set once a reboot is on its way: the disconnection it
// causes isn't a failure, and nothing else should start.
extern bool disconnecting;

// Send log (ble_send.c): the log on its way, the phone it goes to.
extern datalog_t *log_to_send;
extern hci_con_handle_t phone;
extern bool streaming;
extern uint32_t received_at;
// leaving_phone: Send log over, the phone's connection on its way out
// since leaving_since; the dongle's scan starts once it's gone.
extern bool leaving_phone;
extern uint32_t leaving_since;

// ble_errors.c
// ble_error writes what went wrong in a few words, the code after it:
// "<what>: <the code's meaning> (0x56)", or "<what> (HCI 0x..)" for a code it doesn't know.
void ble_error(char *out, size_t size, const char *what, uint8_t code);

// ble.c
void set_phase(phase_t p, const char *status);
void start_bluetooth(void);
void fail_connection(const char *step, const char *error);
// ble_dongle_asleep is a disconnection with the car off (the engine ECU
// silent: DONGLE_ASLEEP_SILENCE_MS, or never answered): the dongle went
// to sleep, no failure. OFFLINE, the radio off, nothing retried: a click
// connects back to it.
void ble_dongle_asleep(void);
#define DONGLE_ASLEEP_SILENCE_MS 5000

// ble_scan.c
void start_dongle_scan(void);
void connect_to_dongle(const bd_addr_t addr, uint8_t type, const char *name);
void handle_advertising_report(uint8_t *packet);

// ble_gatt.c
void discover_dongle_services(void);
void write_pending_command(void);

// ble_send.c
void advertise_to_phone(void);
void back_to_dongle(void);
void start_att_server(void);
