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

// Standard Mode 01 PIDs this dashboard polls. This car's engine ECU lists
// PID 0x24 (wide-range O2 sensor 1 equivalence ratio) as supported
// (0120 bitmap, 2026-11-14); its AFR values aren't cross-checked yet.
#define PID_COOLANT 0x05
#define PID_RPM 0x0C
#define PID_IAT 0x0F
#define PID_LAMBDA 0x24

// GASOLINE_STOICH_AFR turns lambda into a gasoline AFR.
#define GASOLINE_STOICH_AFR 14.7f

// Oil temperature has no standard PID on this car. The FA20 community
// reads it with Toyota's Mode 21 PID 01, asked to the engine ECU (0x7E0):
// byte AC of the answer (the 29th after "61 01") minus 40, in °C. From
// forum Torque setups, checked on this car against another gauge
// (2026-11-14).
#define OBD_ENGINE_ECU 0x7E0
#define OIL_RESPONSE_INDEX 28

// obd_request is the Mode 01 single-frame request for pid.
can_frame_t obd_request(uint8_t pid);

// obd_decode handles an ECU1 single-frame answer (see
// obd_decode_message). It returns false for anything else.
bool obd_decode(const can_frame_t *f, state_t *state, uint32_t now_ms);

// obd_decode_message handles a whole ECU1 answer, mode byte first: Mode
// 01 for the PIDs above (one or several in one answer), Mode 21 PID 01
// for the oil temperature. It returns the first signal it stored, or
// SIG_COUNT for none.
signal_t obd_decode_message(const uint8_t *msg, int len, state_t *state, uint32_t now_ms);

// isotp_t reassembles an ISO-TP message from the frames of one answer:
// a single frame, or a first frame followed by consecutive frames.
typedef struct {
    uint8_t data[64];
    int len, expected;
    uint8_t next_sequence;
} isotp_t;

// isotp_feed adds a frame and returns true once data holds a whole
// message (len bytes, mode byte first).
bool isotp_feed(isotp_t *r, const can_frame_t *f);

// GT86 broadcast frames, seen by a direct CAN tap (MCP2515), never by an
// OBD dongle. IDs and formulas come from community reverse-engineering
// and are unverified on this car.
#define GT86_ID_RPM 0x140
#define GT86_ID_TEMPERATURES 0x360

// gt86_decode handles the broadcast frames above. It returns false for
// anything else.
bool gt86_decode(const can_frame_t *f, state_t *state, uint32_t now_ms);
