#pragma once

// What the host tests share: CHECK. Each test_*.c tests one part of
// src/app; tests.c runs them all.

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "obd/obd.h"

extern int failures;

#define CHECK(cond, ...)                                                                                               \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            failures++;                                                                                                \
            printf("FAIL %s:%d: ", __FILE__, __LINE__);                                                                \
            printf(__VA_ARGS__);                                                                                       \
            printf("\n");                                                                                              \
        }                                                                                                              \
    } while (0)

static inline bool nearly_equal(float a, float b) {
    return fabsf(a - b) < 0.01f;
}

void test_obd_decode(void);
void test_gt86_decode(void);
void test_isotp(void);
void test_flashlog(void);
void test_settings(void);
void test_screen(void);
void test_sensors(void);
void test_mcp_check(void);
void test_limiter_flash(void);
void test_gear_and_trip(void);
void test_orientations(void);
void test_doors(void);
void test_extremes(void);
void test_off_throttle(void);
void test_datalog(void);
void test_wrap(void);
void test_drive_mode(void);
void test_choice_and_idle(void);
void test_live(void);
