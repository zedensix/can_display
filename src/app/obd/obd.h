#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "data/signals.h"

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
// Listed as supported by this car's engine ECU (0100 bitmap BE3FA813,
// 2026-11-14); it read 14 to 17% idling (2026-11-16), wide open isn't known.
#define PID_THROTTLE 0x11
// Also listed as supported by its bitmaps (0100 BE3FA813, 0120 B007B011,
// 0140 FEDC8041), never read yet: their values aren't checked.
#define PID_LOAD 0x04
// The fuel trims, bank 1: the FA20 has a single A/F sensor where both
// banks meet, and the bitmap lists no bank 2 trims (0x08, 0x09). One byte
// each, bank 3 being absent.
#define PID_STFT 0x06
#define PID_LTFT 0x07
#define PID_SPEED 0x0D
#define PID_TIMING 0x0E
#define PID_RAIL 0x23
#define PID_AFR_TARGET 0x44
// Outside air, A - 40 in degrees C.
#define PID_AMBIENT 0x46
// The odometer, 4 bytes in tenths of a km: a newer PID, which this car's
// ECU may not have (its bitmaps from 0x60 on were never asked).
#define PID_ODOMETER 0xA6
// The relative pedal position: 0 to 100% of its travel, where 0x49 (the
// absolute one) stops short of both ends.
#define PID_PEDAL 0x5A

// GASOLINE_STOICH_AFR turns lambda into a gasoline AFR.
#define GASOLINE_STOICH_AFR 14.7f

// Oil temperature has no standard PID on this car. The FA20 community
// reads it with Toyota's Mode 21 PID 01, asked to the engine ECU (0x7E0):
// byte AC of the answer (the 29th after "61 01") minus 40, in °C. From
// forum Torque setups, checked on this car against another gauge
// (2026-11-14).
#define OBD_ENGINE_ECU 0x7E0
#define OIL_RESPONSE_INDEX 28

// The ethanol % has no PID either: the flex-fuel tune keeps it in the
// engine ECU's RAM, a big-endian float at 0xFFF87C7C, read with UDS
// ReadMemoryByAddress (23, 4 address bytes, 1 size byte) asked to
// 0x7E0, answered 63 then the 4 bytes. Found by three RAM scans of one
// tank, matched against OFT on this car (2026-11-16). It's this tune's
// RAM layout: a reflash may move it.
#define ETHANOL_RAM_READ "2314FFF87C7C04"

// obd_build_request is the Mode 01 single-frame request for pid.
can_frame_t obd_build_request(uint8_t pid);

// obd_decode_frame handles an ECU1 single-frame answer (see
// obd_decode_message). It returns false for anything else.
bool obd_decode_frame(const can_frame_t *f, state_t *state, uint32_t now_ms);

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

// isotp_add_frame adds a frame and returns true once data holds a whole
// message (len bytes, mode byte first).
bool isotp_add_frame(isotp_t *r, const can_frame_t *f);

// GT86 broadcast frames, seen by a direct CAN tap (MCP2515), never by an
// OBD dongle. IDs and formulas come from community reverse-engineering
// and are unverified on this car.
#define GT86_ID_RPM 0x140
#define GT86_ID_TEMPERATURES 0x360
// The stability control's mode, broadcast: byte 0 bit 5 set with TRC off
// (TRC OFF, sport + TRC OFF, everything off), byte 0 bit 3 with VSC sport
// (alone or with TRC OFF), byte 3 bit 0 cleared with everything off.
// Byte 1 bit 3 is set in VSC sport and everything off alike: it tells
// neither. Found by two guided Listen CAN runs on this car (2026-11-16):
// the first through on, TRC off and everything off, the second through
// all 5 modes, the same bits each time. Read, not yet checked against
// the cluster's own lamps on a drive.
#define GT86_ID_DRIVE_MODE 0x0D3

// gt86_decode_broadcast handles the broadcast frames above. It returns false for
// anything else.
bool gt86_decode_broadcast(const can_frame_t *f, state_t *state, uint32_t now_ms);
