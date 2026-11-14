#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "flashlog.h"
#include "signals.h"

// The data log: what the gauges and the GPS read, DATALOG_SAMPLE_MS apart
// while the engine runs, in a ring of flash sectors, read back on a
// phone after a session. Each boot is a new session.
//
// A record is DATALOG_RECORD bytes, little-endian:
//   0  u16 session (0xFFFF: erased flash, no record)
//   2  u8  kind: DATALOG_SAMPLE or DATALOG_LAP
//   3  u8  laps completed
//   4  u32 milliseconds since boot
//   8  i16 x 7: RPM, coolant x10, oil x10, intake x10 (degrees C),
//      AFR x100, battery x100 (V), ethanol x10 (%); -32768: no reading
//   22 i32 latitude, 26 i32 longitude (degrees x 1e7)
//   30 u16 speed (km/h x10); latitude and speed 0x7FFFFFFF / 0xFFFF
//      without a GPS fix
// A DATALOG_LAP record is a lap just completed: its latitude field holds
// the lap time in milliseconds, the rest is as in a sample.

#define DATALOG_RECORD 32
#define DATALOG_SAMPLE_MS 200
#define DATALOG_SAMPLE 1
#define DATALOG_LAP 2

typedef struct {
    flash_dev_t dev;
    // pos is where the next record goes; page holds the page it's in
    // until it's full, or flushed.
    uint32_t pos;
    uint16_t session;
    uint8_t page[FLASHLOG_PAGE];
} datalog_t;

// datalog_open finds where the previous run stopped and starts a new
// session after it.
void datalog_open(datalog_t *log, flash_dev_t dev);

void datalog_sample(datalog_t *log, const state_t *state, uint32_t now_ms);
void datalog_lap(datalog_t *log, const state_t *state, uint32_t lap_ms, uint32_t now_ms);

// datalog_flush writes the page in progress, so a power cut loses
// nothing (it's written again as it fills).
void datalog_flush(datalog_t *log);

// datalog_read reads the whole region in ring order, oldest first:
// offset 0 is the oldest byte, dev.size the end. Flush first.
void datalog_read(const datalog_t *log, uint32_t offset, uint8_t *out, size_t len);

// datalog_oldest is the offset, in datalog_read's terms, of the oldest
// record: what's before it is erased flash, not worth sending. dev.size
// for an empty log.
uint32_t datalog_oldest(const datalog_t *log);
