#include "datalog.h"

#include <string.h>

#define NONE16 (-32768)

static uint16_t read_u16_le(const uint8_t *p) {
    return (uint16_t)(p[0] | p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void write_u32_le(uint8_t *p, uint32_t v) {
    write_u16_le(p, (uint16_t)v);
    write_u16_le(p + 2, (uint16_t)(v >> 16));
}

// record_size is the size of the record whose head is head, or of an
// erased slot: the smaller record's.
static uint32_t record_size(const uint8_t head[8]) {
    return read_u16_le(head) != 0xFFFF && head[2] == DATALOG_SAMPLE ? DATALOG_RECORD : DATALOG_RECORD_V1;
}

void datalog_open(datalog_t *log, flash_dev_t dev) {
    *log = (datalog_t){.dev = dev};
    memset(log->page, 0xFF, sizeof log->page);
    // The newest record: the highest session, then the latest time.
    bool found = false;
    uint16_t session = 0;
    uint32_t time = 0, newest_end = 0;
    for (uint32_t at = 0; at + DATALOG_RECORD_V1 <= dev.size;) {
        uint8_t head[8];
        dev.read(dev.ctx, at, head, sizeof head);
        uint16_t s = read_u16_le(head);
        uint32_t t = read_u32_le(head + 4), size = record_size(head);
        if (s != 0xFFFF && (!found || s > session || (s == session && t >= time))) {
            found = true;
            session = s;
            time = t;
            newest_end = at + size;
        }
        at += size;
    }
    if (found) {
        // After an old record, the next 64-byte boundary.
        log->pos = (newest_end + DATALOG_RECORD - 1) / DATALOG_RECORD * DATALOG_RECORD % dev.size;
        log->session = (uint16_t)(session + 1 == 0xFFFF ? 0 : session + 1);
        // The page the next record goes in keeps what's already there.
        uint32_t page = log->pos - log->pos % FLASHLOG_PAGE;
        dev.read(dev.ctx, page, log->page, FLASHLOG_PAGE);
    }
}

static void append_record(datalog_t *log, const uint8_t record[DATALOG_RECORD]) {
    if (log->pos % FLASHLOG_SECTOR == 0) {
        // Entering a sector: whatever the ring left there goes.
        log->dev.erase_sector(log->dev.ctx, log->pos);
        memset(log->page, 0xFF, sizeof log->page);
    }
    memcpy(log->page + log->pos % FLASHLOG_PAGE, record, DATALOG_RECORD);
    log->pos += DATALOG_RECORD;
    if (log->pos % FLASHLOG_PAGE == 0) {
        log->dev.program_page(log->dev.ctx, log->pos - FLASHLOG_PAGE, log->page);
        memset(log->page, 0xFF, sizeof log->page);
        log->pos %= log->dev.size;
    }
}

static int16_t reading_scaled_i16(const state_t *s, signal_t sig, float scale, uint32_t now_ms) {
    const reading_t *r = &s->readings[sig];
    // As the gauges: past their staleness, no reading (the oil, the
    // ethanol, the long term trim and the outside temperature come every 3s).
    if (reading_is_stale(
            r, now_ms, sig == SIG_OIL || sig == SIG_ETHANOL || sig == SIG_LTFT || sig == SIG_AMBIENT ? 10000 : 2000)) {
        return NONE16;
    }
    float v = r->value * scale;
    return (int16_t)(v > 32767 ? 32767 : v < -32767 ? -32767 : v);
}

void datalog_sample(datalog_t *log, const state_t *s, uint32_t now_ms) {
    uint8_t r[DATALOG_RECORD];
    memset(r, 0xFF, sizeof r);
    write_u16_le(r, log->session);
    r[2] = DATALOG_SAMPLE;
    write_u32_le(r + 4, now_ms);
    static const struct {
        signal_t sig;
        float scale;
    } columns[] = {{SIG_RPM, 1},       {SIG_COOLANT, 10}, {SIG_OIL, 10},      {SIG_IAT, 10},         {SIG_AFR, 100},
                   {SIG_BATTERY, 100}, {SIG_ETHANOL, 10}, {SIG_THROTTLE, 10}, {SIG_PEDAL, 10},       {SIG_LOAD, 10},
                   {SIG_TIMING, 10},   {SIG_STFT, 10},    {SIG_LTFT, 10},     {SIG_AFR_TARGET, 100}, {SIG_RAIL, 100},
                   {SIG_SPEED, 1},     {SIG_AMBIENT, 10}};
    for (size_t i = 0; i < sizeof columns / sizeof columns[0]; i++) {
        write_u16_le(r + 8 + 2 * i, (uint16_t)reading_scaled_i16(s, columns[i].sig, columns[i].scale, now_ms));
    }
    append_record(log, r);
}

void datalog_flush(datalog_t *log) {
    if (log->pos % FLASHLOG_PAGE != 0) {
        log->dev.program_page(log->dev.ctx, log->pos - log->pos % FLASHLOG_PAGE, log->page);
    }
}

uint32_t datalog_oldest_record_offset(const datalog_t *log) {
    for (uint32_t offset = 0; offset < log->dev.size; offset += DATALOG_RECORD_V1) {
        uint8_t session[2];
        datalog_read(log, offset, session, sizeof session);
        if (read_u16_le(session) != 0xFFFF) {
            return offset;
        }
    }
    return log->dev.size;
}

void datalog_read(const datalog_t *log, uint32_t offset, uint8_t *out, size_t len) {
    while (len > 0) {
        uint32_t at = (log->pos + offset) % log->dev.size;
        size_t n = log->dev.size - at < len ? log->dev.size - at : len;
        log->dev.read(log->dev.ctx, at, out, n);
        out += n;
        offset += (uint32_t)n;
        len -= n;
    }
}
