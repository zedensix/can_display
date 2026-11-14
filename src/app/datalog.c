#include "datalog.h"

#include <string.h>

#define NONE16 (-32768)

static uint16_t get16(const uint8_t *p) {
    return (uint16_t)(p[0] | p[1] << 8);
}

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v) {
    put16(p, (uint16_t)v);
    put16(p + 2, (uint16_t)(v >> 16));
}

void datalog_open(datalog_t *log, flash_dev_t dev) {
    *log = (datalog_t){.dev = dev};
    memset(log->page, 0xFF, sizeof log->page);
    // The newest record: the highest session, then the latest time.
    bool found = false;
    uint16_t session = 0;
    uint32_t time = 0, newest = 0;
    for (uint32_t at = 0; at + DATALOG_RECORD <= dev.size; at += DATALOG_RECORD) {
        uint8_t head[8];
        dev.read(dev.ctx, at, head, sizeof head);
        uint16_t s = get16(head);
        uint32_t t = get32(head + 4);
        if (s == 0xFFFF) {
            continue;
        }
        if (!found || s > session || (s == session && t >= time)) {
            found = true;
            session = s;
            time = t;
            newest = at;
        }
    }
    if (found) {
        log->pos = (newest + DATALOG_RECORD) % dev.size;
        log->session = (uint16_t)(session + 1 == 0xFFFF ? 0 : session + 1);
        // The page the next record goes in keeps what's already there.
        uint32_t page = log->pos - log->pos % FLASHLOG_PAGE;
        dev.read(dev.ctx, page, log->page, FLASHLOG_PAGE);
    }
}

static void append(datalog_t *log, const uint8_t record[DATALOG_RECORD]) {
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

static int16_t scaled(const state_t *s, signal_t sig, float scale, uint32_t now_ms) {
    const reading_t *r = &s->readings[sig];
    // As the gauges: past their staleness, no reading (the oil comes
    // every 3s).
    if (reading_stale(r, now_ms, sig == SIG_OIL ? 10000 : 2000)) {
        return NONE16;
    }
    float v = r->value * scale;
    return (int16_t)(v > 32767 ? 32767 : v < -32767 ? -32767 : v);
}

void datalog_sample(datalog_t *log, const state_t *s, uint32_t now_ms) {
    uint8_t r[DATALOG_RECORD];
    memset(r, 0xFF, sizeof r);
    put16(r, log->session);
    r[2] = DATALOG_SAMPLE;
    put32(r + 4, now_ms);
    static const struct {
        signal_t sig;
        float scale;
    } columns[7] = {{SIG_RPM, 1},   {SIG_COOLANT, 10},  {SIG_OIL, 10},    {SIG_IAT, 10},
                    {SIG_AFR, 100}, {SIG_BATTERY, 100}, {SIG_ETHANOL, 10}};
    for (int i = 0; i < 7; i++) {
        put16(r + 8 + 2 * i, (uint16_t)scaled(s, columns[i].sig, columns[i].scale, now_ms));
    }
    append(log, r);
}

void datalog_flush(datalog_t *log) {
    if (log->pos % FLASHLOG_PAGE != 0) {
        log->dev.program_page(log->dev.ctx, log->pos - log->pos % FLASHLOG_PAGE, log->page);
    }
}

uint32_t datalog_oldest(const datalog_t *log) {
    for (uint32_t offset = 0; offset < log->dev.size; offset += DATALOG_RECORD) {
        uint8_t session[2];
        datalog_read(log, offset, session, sizeof session);
        if (get16(session) != 0xFFFF) {
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
