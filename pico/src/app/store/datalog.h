#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "data/signals.h"
#include "flashlog.h"

// The data log: what the gauges read, DATALOG_SAMPLE_MS apart while the
// engine runs, in a ring of flash sectors, read back on a
// phone after a session. Each boot is a new session.
//
// A record is DATALOG_RECORD bytes, little-endian:
//   0  u16 session (0xFFFF: erased flash, no record)
//   2  u8  kind: DATALOG_SAMPLE
//   4  u32 milliseconds since boot
//   8  i16 x 16: RPM, coolant x10, oil x10, intake x10 (degrees C),
//      AFR x100, battery x100 (V), ethanol x10 (%), throttle x10 (%),
//      pedal x10 (%), load x10 (%), timing x10 (degrees), short and
//      long term fuel trims x10 (%), commanded AFR x100, rail pressure
//      x100 (MPa), speed (km/h), outside x10 (degrees C);
//      -32768: no reading.
// Byte 3 and bytes 42 to 63: 0xFF, room for 11 more readings. 64 bytes,
// so a record never straddles a 256-byte flash page: about 50 minutes of
// driving in the ring.
//
// A firmware before the tune's readings wrote DATALOG_RECORD_V1 records,
// kind DATALOG_SAMPLE_V1: the first 8 readings, bytes 24 to 31 0xFF (and
// the throttle 0xFFFF, -1, from before it was logged). The ring may still
// hold them, at any 32-byte boundary: walking it, a record's kind says
// its size, and a new record starts at a 64-byte boundary.

#define DATALOG_RECORD 64
#define DATALOG_RECORD_V1 32
#define DATALOG_SAMPLE_MS 200
#define DATALOG_SAMPLE 2
#define DATALOG_SAMPLE_V1 1

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

// datalog_flush writes the page in progress, so a power cut loses
// nothing (it's written again as it fills).
void datalog_flush(datalog_t *log);

// datalog_read reads the whole region in ring order, oldest first:
// offset 0 is the oldest byte, dev.size the end. Flush first.
void datalog_read(const datalog_t *log, uint32_t offset, uint8_t *out, size_t len);

// datalog_oldest_record_offset is the offset, in datalog_read's terms, of the oldest
// record: what's before it is erased flash, not worth sending. dev.size
// for an empty log.
uint32_t datalog_oldest_record_offset(const datalog_t *log);

// datalog_since is what was written after from, where the log was when
// a previous call returned (DATALOG_FROM_OLDEST, or anything past the
// end: every record): its offset and length in datalog_read's terms.
// Returns where the log is now, the next call's from. Flush first. A ring
// gone round past from since gives the newest dev.size bytes' worth.
#define DATALOG_FROM_OLDEST 0xFFFFFFFFu
uint32_t datalog_since(const datalog_t *log, uint32_t from, uint32_t *offset, uint32_t *len);
