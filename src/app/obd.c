#include "obd.h"

can_frame_t obd_request(uint8_t pid) {
    return (can_frame_t){.id = OBD_REQUEST_ID, .dlc = 8, .data = {0x02, 0x01, pid}};
}

bool obd_decode(const can_frame_t *f, state_t *state, uint32_t now_ms) {
    isotp_t r = {0};
    return f->id == OBD_RESPONSE_ECU1 && isotp_feed(&r, f) &&
           obd_decode_message(r.data, r.len, state, now_ms) != SIG_COUNT;
}

signal_t obd_decode_message(const uint8_t *msg, int len, state_t *state, uint32_t now_ms) {
    if (len >= 2 && msg[0] == 0x61 && msg[1] == 0x01) {
        if (len < 2 + OIL_RESPONSE_INDEX + 1) {
            return SIG_COUNT;
        }
        state_set(state, SIG_OIL, (float)msg[2 + OIL_RESPONSE_INDEX] - 40, now_ms);
        return SIG_OIL;
    }
    if (len < 3 || msg[0] != 0x41) {
        return SIG_COUNT;
    }
    const uint8_t *v = &msg[2];
    int value_len = len - 2;
    switch (msg[1]) {
    case PID_RPM:
        if (value_len < 2) {
            return SIG_COUNT;
        }
        state_set(state, SIG_RPM, (float)(v[0] << 8 | v[1]) / 4, now_ms);
        return SIG_RPM;
    case PID_COOLANT:
        state_set(state, SIG_COOLANT, (float)v[0] - 40, now_ms);
        return SIG_COOLANT;
    case PID_LAMBDA:
        if (value_len < 2) {
            return SIG_COUNT;
        }
        state_set(state, SIG_AFR, (float)(v[0] << 8 | v[1]) * 2 / 65536 * GASOLINE_STOICH_AFR, now_ms);
        return SIG_AFR;
    default:
        return SIG_COUNT;
    }
}

bool isotp_feed(isotp_t *r, const can_frame_t *f) {
    if (f->dlc < 2) {
        return false;
    }
    uint8_t pci = f->data[0];
    switch (pci >> 4) {
    case 0: { // single frame
        int len = pci & 0x0F;
        if (len < 1 || 1 + len > f->dlc) {
            return false;
        }
        for (int i = 0; i < len; i++) {
            r->data[i] = f->data[1 + i];
        }
        r->len = len;
        r->expected = 0;
        return true;
    }
    case 1: // first frame: 12-bit length, then 6 bytes
        r->expected = (pci & 0x0F) << 8 | f->data[1];
        if (r->expected > (int)sizeof r->data || f->dlc < 8) {
            r->expected = 0;
            return false;
        }
        for (int i = 0; i < 6; i++) {
            r->data[i] = f->data[2 + i];
        }
        r->len = 6;
        r->next_sequence = 1;
        return false;
    case 2: { // consecutive frame: 4-bit sequence number, then up to 7 bytes
        if (r->expected == 0 || (pci & 0x0F) != r->next_sequence) {
            r->expected = 0;
            return false;
        }
        for (int i = 1; i < f->dlc && r->len < r->expected; i++) {
            r->data[r->len++] = f->data[i];
        }
        r->next_sequence = (r->next_sequence + 1) & 0x0F;
        if (r->len < r->expected) {
            return false;
        }
        r->expected = 0;
        return true;
    }
    default:
        return false;
    }
}

bool gt86_decode(const can_frame_t *f, state_t *state, uint32_t now_ms) {
    if (f->dlc < 4) {
        return false;
    }
    switch (f->id) {
    case GT86_ID_RPM:
        state_set(state, SIG_RPM, (float)((f->data[2] | f->data[3] << 8) & 0x3FFF), now_ms);
        return true;
    case GT86_ID_TEMPERATURES:
        state_set(state, SIG_OIL, (float)f->data[2] - 40, now_ms);
        state_set(state, SIG_COOLANT, (float)f->data[3] - 40, now_ms);
        return true;
    default:
        return false;
    }
}
