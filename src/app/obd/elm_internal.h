#pragma once

// What the ELM327 session's files share. elm327.c runs the session: init,
// probes, each answer to its phase, timeouts; elm_poll.c polls the gauges;
// elm_mode.c reads the drive mode off the bus; elm_scan.c scans the
// engine ECU's RAM; elm_parse.c parses the dongle's lines.

#include "elm327.h"

// The raw answers to these are logged: the first thing to look at when
// the dash stays at "--". 0100, 0120, 0140 and 01A0 are the bitmaps of
// the PIDs the engine ECU supports (01A0: whether it has the gear, 0xA4,
// and the odometer, 0xA6). The multi-PID requests ask for 6 PIDs at
// once, the most one request takes: MULTI_POLL_FAST what moves with the
// foot (RPM, AFR, throttle, pedal, load, timing), MULTI_POLL_SLOW the
// rest (coolant, intake, speed, commanded AFR, rail pressure, short term
// trim). If the ECU answers both, polling alternates them (a round trip
// over BLE each instead of one per PID), with the response count digit
// like 010C1. This dongle (IOS-Vlink) counts frames, not answers: a "1"
// got it to stop after the first of the answer's frames (log,
// 2026-11-16). Either answer (17 and 15 bytes) takes 3 frames.
#define MULTI_POLL_FAST "010C24115A040E"
#define MULTI_POLL_SLOW "01050F0D442306"
#define MULTI_POLL_FRAMES "3"

// elm327.c
void send_command(elm_t *e, const char *command, uint32_t timeout_ms, uint32_t now_ms);
void fail_session(elm_t *e, const char *step, const char *error);
void log_command_answer(elm_t *e, const char *command, char *lines[], int n);

// elm_poll.c
void start_polling(elm_t *e, uint32_t now_ms);
void send_next_poll_command(elm_t *e, uint32_t now_ms);
void handle_poll_answer(elm_t *e, char *lines[], int n, uint32_t now_ms);
// enable_multi_pid_if_answered, given the answer to a multi-PID probe,
// turns multi_pid on if MULTI_POLL_FAST's holds the RPM, off if
// MULTI_POLL_SLOW's doesn't hold the coolant (the ECU took neither
// request: it answers what it supports of the rest).
void enable_multi_pid_if_answered(elm_t *e, char *lines[], int n);

// elm_mode.c
// mode_read_due tells whether the drive mode's monitor should start.
bool mode_read_due(const elm_t *e, uint32_t now_ms);
void send_next_mode_command(elm_t *e, uint32_t now_ms);
// mode_monitor_frame takes a frame the monitor printed; true once it had the mode.
bool mode_monitor_frame(elm_t *e, const can_frame_t *f, uint32_t now_ms);
// mode_monitor_over counts a monitor that ended, with the frame or without.
void mode_monitor_over(elm_t *e);

// elm_scan.c
void start_ram_scan(elm_t *e, uint32_t now_ms);
void finish_ram_scan(elm_t *e, uint32_t now_ms);
void handle_ram_scan_answer(elm_t *e, char *lines[], int n, uint32_t now_ms);
