#pragma once

#include <stdbool.h>
#include <stdint.h>

// signal_t names every value the dashboard shows.
typedef enum {
    SIG_RPM,
    SIG_COOLANT,
    SIG_IAT,
    SIG_OIL,
    SIG_BATTERY,
    SIG_AFR,
    SIG_ETHANOL,
    SIG_THROTTLE, // %, absolute: not shown, it tells the AFR alarm a deceleration
    SIG_PEDAL,    // %, the accelerator pedal: what the driver asks, the throttle what the ECU does
    SIG_LOAD,     // %, the engine load the ECU computes
    // The rest is logged, not shown: what checks the tune.
    SIG_TIMING,     // degrees before top dead center
    SIG_STFT,       // %, short term fuel trim
    SIG_LTFT,       // %, long term fuel trim
    SIG_AFR_TARGET, // the AFR the ECU commands, to hold the AFR against
    SIG_RAIL,       // MPa, the direct injection rail's pressure
    SIG_SPEED,      // km/h
    SIG_AMBIENT,    // degrees C, outside
    SIG_ODOMETER,   // km, the car's own, if its ECU knows it (PID 0xA6)
    SIG_COUNT,
} signal_t;

// reading_t is one signal's latest value, and the lowest and highest
// since boot (the session's extremes). Times are milliseconds since boot.
typedef struct {
    float value;
    bool valid;
    uint32_t updated_ms;
    float min, max;
} reading_t;

// wait_t is what the data source offers the driver while it isn't
// delivering, with Settings always beside it (to switch to the CAN source
// when Bluetooth won't do): WAIT_CHOICE, Connect (READY, OFFLINE);
// WAIT_RETRY, Retry (on the way to the dongle, or a failure);
// WAIT_CANCEL, Cancel (a RAM scan, a CAN listen); WAIT_STEP, Next and
// Cancel in place of Settings (a CAN listen's prompt). WAIT_NONE while
// it's delivering, or busy with something of its own.
typedef enum { WAIT_NONE, WAIT_CHOICE, WAIT_RETRY, WAIT_CANCEL, WAIT_STEP } wait_t;

// drive_mode_t is the stability control's mode, from its two buttons:
// everything on; TRC off (a short press on TRC OFF); VSC SPORT (its own
// button); both; everything off (TRC OFF held a few seconds: VSC off).
typedef enum { DRIVE_NORMAL, DRIVE_TRC_OFF, DRIVE_SPORT, DRIVE_SPORT_TRC_OFF, DRIVE_VSC_OFF, DRIVE_MODES } drive_mode_t;

// state_t is everything the UI draws. status is the data source's own
// report while it isn't delivering (scan, connect, failure): while it
// isn't empty, the gauges page shows it instead of the tiles.
typedef struct {
    reading_t readings[SIG_COUNT];
    uint32_t throttle_closed_ms; // the last throttle reading that counted as closed
    // trip_km is the distance since boot, the speed readings added up.
    float trip_km;
    wait_t wait;
    // drive_mode as last read, at drive_mode_ms; drive_mode_read: ever.
    drive_mode_t drive_mode;
    uint32_t drive_mode_ms;
    bool drive_mode_read;
    // doors, a DOOR_* bit each open, as last read, at doors_ms; doors_read: ever.
    uint8_t doors;
    uint32_t doors_ms;
    bool doors_read;
    char status[192];
} state_t;

void state_set_reading(state_t *s, signal_t sig, float value, uint32_t now_ms);
void state_set_status(state_t *s, const char *status);

// reading_is_stale is true for a reading never set, or older than max_age_ms:
// the UI shows "--" rather than a frozen or zero value.
bool reading_is_stale(const reading_t *r, uint32_t now_ms, uint32_t max_age_ms);

// THROTTLE_CLOSED_MARGIN is how far above the lowest throttle seen since
// boot still counts as closed: the ECU's closed position isn't 0%, and
// rather than a guessed figure, the session's minimum (idling, lifting
// off) is the closed throttle.
#define THROTTLE_CLOSED_MARGIN 5

// THROTTLE_REOPEN_HOLD_MS is how long a throttle still counts as closed
// once opened again: the AFR reading lags the fuel coming back.
#define THROTTLE_REOPEN_HOLD_MS 1000

// state_off_throttle is true while a fresh throttle reading says the foot
// is off, or was THROTTLE_REOPEN_HOLD_MS ago: decelerating, the ECU cuts
// the fuel and the AFR reads lean (20.4 on this car), or idling. Without
// a throttle reading, false.
bool state_off_throttle(const state_t *s, uint32_t now_ms, uint32_t max_age_ms);

// TRIP_MAX_GAP_MS: speed readings further apart than that (the source
// busy elsewhere) don't add up the time between them to the trip.
#define TRIP_MAX_GAP_MS 2000

// The gear comes from how fast the engine turns for the car's speed:
// RPM per km/h is GEAR_RPM_PER_KMH times the gear's ratio, for the
// GT86's manual gearbox and its 4.1 final drive. The speed the ECU gives
// counts wheel turns at the size it was set for, the stock 215/45R17
// (1.964m around, assumed): the tyres fitted (225/40R18 here, 2.002m)
// change the true speed, not this ratio. The gears are 20% apart or
// more: off by more than GEAR_TOLERANCE from every one, the clutch is in
// (or slipping).
#define GEAR_RPM_PER_KMH (1000.0f / 60 / 1.964f * 4.1f)
#define GEAR_TOLERANCE 0.08f
#define GEAR_MIN_KMH 5
#define GEARS 6
extern const float gear_ratios[GEARS];

// state_gear is the gear engaged, 1 to 6, from fresh RPM and speed
// readings (max_age_ms); 0 for none: stopped, the clutch in, or no
// readings.
int state_gear(const state_t *s, uint32_t now_ms, uint32_t max_age_ms);

void state_set_drive_mode(state_t *s, drive_mode_t mode, uint32_t now_ms);

// DRIVE_MODE_STALE_MS: a mode read longer ago than that no longer counts
// (the dongle reads it every few seconds, the CAN bus many times a second).
#define DRIVE_MODE_STALE_MS 10000
// DRIVE_MODE_MIN_RPM: with the engine off (contact on), 0x0D3 reads as
// everything off (byte 3 bit 0 cleared: the VSC isn't running), so the
// mode only counts with the engine running.
#define DRIVE_MODE_MIN_RPM 400

// The doors and the boot, a bit each in state_t.doors (0x375, obd.h).
enum { DOOR_DRIVER = 1, DOOR_PASSENGER = 2, DOOR_BOOT = 4 };
// DOORS_STALE_MS: doors read longer ago than that count as shut.
#define DOORS_STALE_MS 10000
void state_set_doors(state_t *s, uint8_t doors, uint32_t now_ms);
// state_doors is the DOOR_* bits open, 0 without a recent reading.
uint8_t state_doors(const state_t *s, uint32_t now_ms);

// state_drive_mode is the mode the UI goes by: the last one read, or
// DRIVE_NORMAL without a recent one or with the engine off.
drive_mode_t state_drive_mode(const state_t *s, uint32_t now_ms);
