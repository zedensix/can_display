#pragma once

// What the ELM327 session's files share. elm327.c runs the session: init,
// probes, each answer to its phase, timeouts; elm_poll.c polls the gauges;
// elm_mode.c reads the drive mode off the bus; elm_scan.c scans the engine ECU's RAM; elm_listen.c listens to the CAN
// bus; elm_parse.c parses the dongle's lines.

#include "elm327.h"

// The raw answers to these are logged: the first thing to look at when
// the dash stays at "--". 0100, 0120 and 0140 are the bitmaps of the
// PIDs the engine ECU supports. MULTI_POLL asks for every polled PID at
// once: if its answer holds them all, polling uses it (one round trip
// over BLE instead of five), with the response count digit like 010C1.
// This dongle (IOS-Vlink) counts frames, not answers: a "1" got it to
// stop after the first of the answer's frames (log, 2026-11-16). The 15
// bytes take 3 frames.
#define MULTI_POLL "010C24050F11"
#define MULTI_POLL_FRAMES "3"

// elm327.c
void send_command(elm_t *e, const char *command, uint32_t timeout_ms, uint32_t now_ms);
void fail_session(elm_t *e, const char *step, const char *error);
void log_command_answer(elm_t *e, const char *command, char *lines[], int n);

// elm_poll.c
void start_polling(elm_t *e, uint32_t now_ms);
void send_next_poll_command(elm_t *e, uint32_t now_ms);
void handle_poll_answer(elm_t *e, char *lines[], int n, uint32_t now_ms);
// enable_multi_pid_if_answered turns multi_pid on if lines, the answer to MULTI_POLL,
// hold every PID it asked for.
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

// elm_listen.c
void start_can_listen(elm_t *e, uint32_t now_ms);
void handle_can_listen_answer(elm_t *e, uint32_t now_ms);
