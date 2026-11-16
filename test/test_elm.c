#include "check.h"

void test_elm_session(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_callbacks_t cb = {fake_write, fake_log, fake_ready, fake_failed, &d};
    elm_start_session(&e, &s, cb, 0);
    CHECK(strcmp(last_sent(&d), "ATZ\r") == 0, "first command %s, want ATZ", last_sent(&d));
    feed_dongle_answer(&e, "ATZ\r\rELM327 v2.3\r\r>", 10);
    for (int i = 0; i < 7; i++) {
        feed_dongle_answer(&e, "OK\r\r>", 20);
    }
    CHECK(strcmp(last_sent(&d), "ATI\r") == 0, "after init sent %s, want the ATI probe", last_sent(&d));
    for (int i = 0; i < 7; i++) {
        feed_dongle_answer(&e, "whatever\r>", 30);
    }
    CHECK(d.ready == 1, "ready called %d times after the probes", d.ready);
    CHECK(strcmp(last_sent(&d), "ATRV\r") == 0, "first poll %s, want ATRV", last_sent(&d));
    feed_dongle_answer(&e, "12.4V\r>", 40);
    CHECK(nearly_equal(s.readings[SIG_BATTERY].value, 12.4f), "battery %f, want 12.4",
          (double)s.readings[SIG_BATTERY].value);
    CHECK(strcmp(last_sent(&d), "010C1\r") == 0, "then %s, want 010C1", last_sent(&d));
    feed_dongle_answer(&e, "?\r>", 50);
    CHECK(strcmp(last_sent(&d), "010C\r") == 0, "after ? sent %s, want 010C without the count digit", last_sent(&d));
    feed_dongle_answer(&e, "7E804410C1AF8\r>", 60);
    CHECK(nearly_equal(s.readings[SIG_RPM].value, 1726), "rpm %f, want 1726", (double)s.readings[SIG_RPM].value);
    CHECK(e.ecu_ms == 60, "ecu_ms %u after the ECU answered at 60, want 60 (car-off detection)", (unsigned)e.ecu_ms);
    CHECK(strcmp(last_sent(&d), "0124\r") == 0, "then %s, want 0124", last_sent(&d));

    int before = d.sent_count;
    elm_check_timers(&e, 60 + ELM_TIMEOUT_MS + 1);
    CHECK(d.sent_count == before + 1 && strcmp(last_sent(&d), "0124\r") == 0, "a timeout didn't resend 0124");
    elm_check_timers(&e, 60 + 2 * ELM_TIMEOUT_MS + 2);
    elm_check_timers(&e, 60 + 3 * ELM_TIMEOUT_MS + 3);
    CHECK(d.failed == 1 && strcmp(d.failed_step, "OBD poll") == 0, "after %d retries: failed %d, step %s", ELM_RETRIES,
          d.failed, d.failed_step);
}

// oil_answer is a 2101 answer: 0x2B bytes in a first frame and 6
// consecutive frames, the oil byte (AC) holding 130, i.e. 90°C.
static const char *oil_answer = "7E8102B610100000000\r"
                                "7E82100000000000000\r"
                                "7E82200000000000000\r"
                                "7E82300000000000000\r"
                                "7E82400000082000000\r"
                                "7E82500000000000000\r"
                                "7E82600000000000000\r>";

void test_elm_oil(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    feed_dongle_answer(&e, "12.4V\r>", 3); // ATRV, the first poll
    feed_dongle_answer(&e, "NO DATA\r>", ELM_ENGINE_INTERVAL_MS + 3);
    CHECK(strcmp(last_sent(&d), "ATSH7E0\r") == 0, "after %dms sent %s, want the engine ECU sequence's ATSH7E0",
          ELM_ENGINE_INTERVAL_MS, last_sent(&d));
    feed_dongle_answer(&e, "OK\r>", ELM_ENGINE_INTERVAL_MS + 4);
    CHECK(strcmp(last_sent(&d), "2101\r") == 0, "then %s, want 2101", last_sent(&d));
    feed_dongle_answer(&e, oil_answer, ELM_ENGINE_INTERVAL_MS + 5);
    CHECK(nearly_equal(s.readings[SIG_OIL].value, 90), "oil %f, want 90", (double)s.readings[SIG_OIL].value);
    CHECK(strcmp(last_sent(&d), ETHANOL_RAM_READ "\r") == 0, "then %s, want the ethanol's RAM read", last_sent(&d));
    feed_dongle_answer(&e, "7E8056342858000\r>", ELM_ENGINE_INTERVAL_MS + 6); // 66.75 as a big-endian float
    CHECK(nearly_equal(s.readings[SIG_ETHANOL].value, 66.75f), "ethanol %f, want 66.75",
          (double)s.readings[SIG_ETHANOL].value);
    CHECK(strcmp(last_sent(&d), "ATSH7DF\r") == 0, "then %s, want the header back to ATSH7DF", last_sent(&d));

    for (int miss = 0; miss < ELM_ENGINE_MAX_MISSES; miss++) {
        uint32_t t = (uint32_t)(miss + 2) * (ELM_ENGINE_INTERVAL_MS + 10);
        feed_dongle_answer(&e, "OK\r>", t);
        while (strcmp(last_sent(&d), "ATSH7E0\r") != 0) {
            feed_dongle_answer(&e, "NO DATA\r>", t);
        }
        feed_dongle_answer(&e, "OK\r>", t);
        feed_dongle_answer(&e, "NO DATA\r>", t); // 2101 unanswered
    }
    int before = d.sent_count;
    for (int i = 0; i < 20; i++) {
        feed_dongle_answer(&e, "OK\r>", 100000 + (uint32_t)i * ELM_ENGINE_INTERVAL_MS);
    }
    bool asked = false;
    for (int i = before; i < d.sent_count; i++) {
        asked = asked || strcmp(d.sent[i], "2101\r") == 0;
    }
    CHECK(!asked, "2101 still asked after %d answers without the oil temperature", ELM_ENGINE_MAX_MISSES);
    CHECK(strcmp(last_sent(&d), "ATSH7E0\r") != 0, "the header still switched with both reads given up");
}

// With the car off at boot nothing answers: 2101 must still be asked,
// so the oil temperature shows once the car starts.
void test_elm_oil_car_off(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v2.3\r>", 1);
    for (int i = 0; i < 7 + 7; i++) {
        feed_dongle_answer(&e, "OK\r>", 2);
    }
    for (uint32_t t = 3; t < 10 * ELM_ENGINE_INTERVAL_MS; t += 100) {
        feed_dongle_answer(
            &e,
            strcmp(last_sent(&d), "ATSH7E0\r") == 0 || strcmp(last_sent(&d), "ATSH7DF\r") == 0 ? "OK\r>" : "NO DATA\r>",
            t);
    }
    int asked = 0;
    for (int i = 0; i < d.sent_count; i++) {
        asked += strcmp(d.sent[i], "2101\r") == 0;
    }
    CHECK(asked > ELM_ENGINE_MAX_MISSES, "2101 asked %d times with the car off, want it kept", asked);
}

void test_elm_init_rejected(void) {
    fake_dongle_t d = {0};
    state_t s = {0};
    elm_t e;
    elm_start_session(&e, &s, (elm_callbacks_t){fake_write, fake_log, fake_ready, fake_failed, &d}, 0);
    feed_dongle_answer(&e, "ELM327 v1.5\r>", 1);
    feed_dongle_answer(&e, "?\r>", 2);
    CHECK(d.failed == 1 && strcmp(d.failed_step, "ELM327 init") == 0, "a rejected ATE0 gave failed %d, step %s",
          d.failed, d.failed_step);
}
