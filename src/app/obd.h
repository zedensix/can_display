#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "signals.h"

// can_frame_t is a classic CAN frame with an 11bit ID.
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} can_frame_t;

// ISO 15765-4 OBD-II on CAN: requests go to the functional (broadcast)
// address, the engine ECU answers from 0x7E8.
#define OBD_REQUEST_ID 0x7DF
#define OBD_RESPONSE_ECU1 0x7E8

// Standard Mode 01 PIDs this dashboard polls. PID 0x24 (wide-range O2
// sensor 1 equivalence ratio) is speculative on the GT86: unverified.
#define PID_COOLANT 0x05
#define PID_RPM 0x0C
#define PID_LAMBDA 0x24

// GASOLINE_STOICH_AFR turns lambda into a gasoline AFR.
#define GASOLINE_STOICH_AFR 14.7f

// obd_request is the Mode 01 single-frame request for pid.
can_frame_t obd_request(uint8_t pid);

// obd_decode handles an ECU1 Mode 01 single-frame answer to one of the
// PIDs above and stores the value. It returns false for anything else.
bool obd_decode(const can_frame_t *f, state_t *state, uint32_t now_ms);

// GT86 broadcast frames, seen by a direct CAN tap (MCP2515), never by an
// OBD dongle. IDs and formulas come from community reverse-engineering
// and are unverified on this car.
#define GT86_ID_RPM 0x140
#define GT86_ID_TEMPERATURES 0x360

// gt86_decode handles the broadcast frames above. It returns false for
// anything else.
bool gt86_decode(const can_frame_t *f, state_t *state, uint32_t now_ms);
