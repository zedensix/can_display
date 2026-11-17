#include "check.h"
#include "obd/sniff.h"

// session_polling starts a session and answers its init and probes.
static void session_polling(elm_t *e, state_t *s, fake_dongle_t *d) {
    elm_start_session(e, s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, d}, 0);
    feed_dongle_answer(e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 9; i++) {
        feed_dongle_answer(e, "OK\r>", 2);
    }
}

// answer_listen_window answers one window of a listen: the ATCF, then the
// monitor, frames printed until the dwell is over and the stop.
static void answer_listen_window(elm_t *e, uint32_t *t, const char *frames) {
    feed_dongle_answer(e, "OK\r>", *t);
    feed_dongle_answer(e, frames, *t);
    *t += ELM_LISTEN_DWELL_MS;
    elm_check_timers(e, *t);
    feed_dongle_answer(e, "STOPPED\r>", *t + 1);
    *t += 2;
}

// A guided listen through the doors, the stalk and the cruise control:
// each state's bits found, the noise left out.
void test_elm_listen(void) {
    static fake_dongle_t d;
    memset(&d, 0, sizeof d);
    state_t s = {0};
    static elm_t e;
    session_polling(&e, &s, &d);
    elm_request_can_listen(&e);
    feed_dongle_answer(&e, "12.4V\r>", 3); // the poll in flight
    CHECK(strcmp(last_sent(&d), "ATCAF0\r") == 0, "listening sent %s, want ATCAF0", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 4);
    CHECK(strcmp(last_sent(&d), "ATCM700\r") == 0, "then %s, want ATCM700", last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", 5);
    CHECK(strstr(s.status, "LISTEN 1/15") && s.wait == WAIT_STEP && !e.waiting,
          "the first step doesn't wait for Next: status %s, wait %d", s.status, s.wait);

    // 1A0 is a rolling counter; 3B0 the state alone; 2C0 a counter and,
    // in its next byte, a bit per state; 4D0 a temperature, a degree more
    // from step 4: it drifts, the steps back to shut disagree.
    static const char *const state_3b0[SNIFF_STATES] = {"AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1"};
    static const char *const state_2c0[SNIFF_STATES] = {"00", "01", "02", "04", "08", "10", "20", "40"};
    uint32_t t = 10;
    for (int step = 0; step < SNIFF_STEPS; step++) {
        CHECK(elm_listen_click(&e, t), "step %d: Next wasn't taken", step + 1);
        CHECK(s.wait == WAIT_CANCEL, "step %d: no Cancel while listening", step + 1);
        sniff_state_t state = sniff_steps[step].state;
        for (int w = 0; w < ELM_LISTEN_WINDOWS * ELM_LISTEN_ROUNDS; w++) {
            CHECK(w == 0 || !elm_listen_click(&e, t), "Next was taken while listening");
            char frames[96];
            snprintf(frames, sizeof frames, "1A0%02X\r3B000%s\r2C0%02X%s\r4D0%02X\rNO DATA\r", w & 0x0F,
                     state_3b0[state], w & 0x0F, state_2c0[state], 0x50 + (step >= 3));
            answer_listen_window(&e, &t, frames);
        }
    }
    CHECK(strcmp(last_sent(&d), "ATZ\r") == 0, "after listening sent %s, want ATZ (session restart)", last_sent(&d));
    CHECK(s.wait == WAIT_NONE, "a button still offered after the listen");
    CHECK(strstr(d.logged, "sniff: 3B0 bits 001F: S 00AA D 00AB P 00AC B 00AD U 00AE W 00AF C 00B0 M 00B1") != NULL,
          "3B0 not reported as a candidate:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 2C0 bits 007F: S 0F00 D 0F01 P 0F02 B 0F04 U 0F08 W 0F10 C 0F20 M 0F40") != NULL,
          "2C0's flags, beside a counter, not reported:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 1A0") == NULL, "1A0, a counter, reported:\n%s", d.logged);
    CHECK(strstr(d.logged, "sniff: 4D0") == NULL, "4D0, drifting, reported:\n%s", d.logged);
}

// A listen cancelled at a prompt ends at once; cancelled while the dongle
// monitors, it stops the monitor and ends at its answer.
void test_elm_listen_cancel(void) {
    static fake_dongle_t d;
    memset(&d, 0, sizeof d);
    state_t s = {0};
    static elm_t e;
    session_polling(&e, &s, &d);
    elm_request_can_listen(&e);
    feed_dongle_answer(&e, "12.4V\r>", 3);
    feed_dongle_answer(&e, "OK\r>", 4);
    feed_dongle_answer(&e, "OK\r>", 5);
    CHECK(elm_cancel_listen(&e, 6) && strcmp(last_sent(&d), "ATZ\r") == 0 && s.wait == WAIT_NONE,
          "cancelled at a prompt: sent %s, wait %d", last_sent(&d), s.wait);

    memset(&d, 0, sizeof d);
    session_polling(&e, &s, &d);
    elm_request_can_listen(&e);
    feed_dongle_answer(&e, "12.4V\r>", 3);
    feed_dongle_answer(&e, "OK\r>", 4);
    feed_dongle_answer(&e, "OK\r>", 5);
    elm_listen_click(&e, 6);
    feed_dongle_answer(&e, "OK\r>", 7); // ATCF: the monitor starts
    CHECK(e.monitoring, "no monitor running");
    CHECK(elm_cancel_listen(&e, 8) && strcmp(last_sent(&d), "\r") == 0, "the monitor wasn't stopped: %s",
          last_sent(&d));
    CHECK(!elm_cancel_listen(&e, 8), "a second cancel taken");
    feed_dongle_answer(&e, "STOPPED\r>", 9);
    CHECK(strcmp(last_sent(&d), "ATZ\r") == 0 && s.wait == WAIT_NONE, "after the stop sent %s, want ATZ",
          last_sent(&d));
}
