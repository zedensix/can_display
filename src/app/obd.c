#include "obd.h"

can_frame_t obd_request(uint8_t pid) {
    return (can_frame_t){.id = OBD_REQUEST_ID, .dlc = 8, .data = {0x02, 0x01, pid}};
}

bool obd_decode(const can_frame_t *f, state_t *state, uint32_t now_ms) {
    if (f->id != OBD_RESPONSE_ECU1 || f->dlc < 4) {
        return false;
    }
    // ISO-TP single frame: high nibble 0, low nibble = bytes that follow
    // (mode, PID, then the value).
    uint8_t pci = f->data[0];
    int length = pci & 0x0F;
    if ((pci & 0xF0) != 0 || length < 3 || 1 + length > f->dlc || f->data[1] != 0x41) {
        return false;
    }
    const uint8_t *v = &f->data[3];
    int value_len = length - 2;
    switch (f->data[2]) {
    case PID_RPM:
        if (value_len < 2) {
            return false;
        }
        state_set(state, SIG_RPM, (float)(v[0] << 8 | v[1]) / 4, now_ms);
        return true;
    case PID_COOLANT:
        state_set(state, SIG_COOLANT, (float)v[0] - 40, now_ms);
        return true;
    case PID_LAMBDA:
        if (value_len < 2) {
            return false;
        }
        state_set(state, SIG_AFR, (float)(v[0] << 8 | v[1]) * 2 / 65536 * GASOLINE_STOICH_AFR, now_ms);
        return true;
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
