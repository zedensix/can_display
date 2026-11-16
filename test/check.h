#pragma once

// What the host tests share: CHECK, and a fake dongle for the ELM327
// session's tests. Each test_*.c tests one part of src/app; tests.c runs
// them all.

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "obd/elm327.h"

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

// fake_dongle records what the session sends and lets a test answer.
typedef struct {
    char sent[512][16];
    int sent_count;
    int ready, failed;
    char failed_step[32];
    char logged[4096];
} fake_dongle_t;

void fake_write(void *ctx, const char *command);
void fake_log(void *ctx, const char *line);
void fake_ready(void *ctx);
void fake_failed(void *ctx, const char *step, const char *error);
void feed_dongle_answer(elm_t *e, const char *text, uint32_t now);
const char *last_sent(fake_dongle_t *d);

void test_obd_decode(void);
void test_gt86_decode(void);
void test_elm_parsing(void);
void test_elm_session(void);
void test_elm_oil(void);
void test_elm_oil_car_off(void);
void test_elm_scan(void);
void test_elm_multi_pid(void);
void test_elm_listen(void);
void test_isotp(void);
void test_elm_init_rejected(void);
void test_flashlog(void);
void test_settings(void);
void test_screen(void);
void test_sensors(void);
void test_extremes(void);
void test_off_throttle(void);
void test_datalog(void);
void test_wrap(void);
void test_drive_mode(void);
void test_elm_drive_mode(void);
void test_choice_and_idle(void);
