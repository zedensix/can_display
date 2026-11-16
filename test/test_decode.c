#include "check.h"
#include "obd/obd.h"

void test_obd_decode(void) {
    state_t s = {0};
    can_frame_t rpm = {OBD_RESPONSE_ECU1, 8, {0x04, 0x41, 0x0C, 0x1A, 0xF8}};
    CHECK(obd_decode_frame(&rpm, &s, 1) && nearly_equal(s.readings[SIG_RPM].value, 1726), "rpm = %f, want 1726",
          (double)s.readings[SIG_RPM].value);
    can_frame_t coolant = {OBD_RESPONSE_ECU1, 8, {0x03, 0x41, 0x05, 0x7B}};
    CHECK(obd_decode_frame(&coolant, &s, 1) && nearly_equal(s.readings[SIG_COOLANT].value, 83), "coolant = %f, want 83",
          (double)s.readings[SIG_COOLANT].value);
    can_frame_t lambda = {OBD_RESPONSE_ECU1, 8, {0x06, 0x41, 0x24, 0x80, 0x00, 0x00, 0x00}};
    CHECK(obd_decode_frame(&lambda, &s, 1) && nearly_equal(s.readings[SIG_AFR].value, 14.7f), "afr = %f, want 14.7",
          (double)s.readings[SIG_AFR].value);
    can_frame_t throttle = {OBD_RESPONSE_ECU1, 8, {0x03, 0x41, 0x11, 0x33}};
    CHECK(obd_decode_frame(&throttle, &s, 1) && nearly_equal(s.readings[SIG_THROTTLE].value, 20),
          "throttle = %f, want 20", (double)s.readings[SIG_THROTTLE].value);
    can_frame_t ethanol = {OBD_RESPONSE_ECU1, 8, {0x05, 0x63, 0x42, 0x85, 0x80, 0x00}};
    CHECK(obd_decode_frame(&ethanol, &s, 1) && nearly_equal(s.readings[SIG_ETHANOL].value, 66.75f),
          "ethanol = %f, want 66.75", (double)s.readings[SIG_ETHANOL].value);
    can_frame_t garbage = {OBD_RESPONSE_ECU1, 8, {0x05, 0x63, 0x7F, 0xC0, 0x00, 0x00}}; // NaN
    CHECK(!obd_decode_frame(&garbage, &s, 2) && s.readings[SIG_ETHANOL].updated_ms == 1, "a NaN ethanol %% was taken");
    can_frame_t other_ecu = {0x7E9, 8, {0x03, 0x41, 0x05, 0x7B}};
    CHECK(!obd_decode_frame(&other_ecu, &s, 1), "a frame from 0x7E9 was decoded");
    can_frame_t multi = {OBD_RESPONSE_ECU1, 8, {0x10, 0x14, 0x49, 0x02}};
    CHECK(!obd_decode_frame(&multi, &s, 1), "an ISO-TP first frame was decoded");
}

void test_gt86_decode(void) {
    state_t s = {0};
    can_frame_t temps = {GT86_ID_TEMPERATURES, 8, {0, 0, 130, 125}};
    CHECK(gt86_decode_broadcast(&temps, &s, 1) && nearly_equal(s.readings[SIG_OIL].value, 90) &&
              nearly_equal(s.readings[SIG_COOLANT].value, 85),
          "0x360 decoded oil %f coolant %f, want 90 and 85", (double)s.readings[SIG_OIL].value,
          (double)s.readings[SIG_COOLANT].value);
    can_frame_t rpm = {GT86_ID_RPM, 8, {0, 0, 0xB8, 0xCB}};
    CHECK(gt86_decode_broadcast(&rpm, &s, 1) && nearly_equal(s.readings[SIG_RPM].value, 3000),
          "0x140 rpm = %f, want 3000", (double)s.readings[SIG_RPM].value);
}

void test_elm_parsing(void) {
    can_frame_t f;
    CHECK(elm_parse_frame("7E8 03 41 05 7B", &f) && f.id == 0x7E8 && f.dlc == 4 && f.data[3] == 0x7B,
          "spaced frame line misparsed");
    CHECK(elm_parse_frame("7E80341057B", &f) && f.dlc == 4, "compact frame line misparsed");
    CHECK(!elm_parse_frame("NO DATA", &f), "NO DATA parsed as a frame");
    CHECK(!elm_parse_frame("18DAF110034105", &f), "29bit frame parsed as 11bit");
    float v;
    CHECK(elm_parse_voltage("12.6V", &v) && nearly_equal(v, 12.6f), "12.6V misparsed");
    CHECK(!elm_parse_voltage("?", &v), "? parsed as a voltage");

    char raw[] = "ATZ\r\r\rELM327 v2.3\r\rSEARCHING...\r 7E8 03 41 05 7B \r";
    char *lines[ELM_MAX_LINES];
    int n = elm_split_lines(raw, "ATZ", lines, ELM_MAX_LINES);
    CHECK(n == 2 && strcmp(lines[0], "ELM327 v2.3") == 0 && strcmp(lines[1], "7E8 03 41 05 7B") == 0,
          "split gave %d lines", n);
}

void test_isotp(void) {
    isotp_t r = {0};
    can_frame_t first = {OBD_RESPONSE_ECU1, 8, {0x10, 0x0A, 0x61, 0x01, 1, 2, 3, 4}};
    can_frame_t next = {OBD_RESPONSE_ECU1, 8, {0x21, 5, 6, 7, 8, 9, 10, 11}};
    CHECK(!isotp_add_frame(&r, &first), "a first frame alone completed a message");
    CHECK(isotp_add_frame(&r, &next) && r.len == 10 && r.data[0] == 0x61 && r.data[9] == 8,
          "first + consecutive gave len %d", r.len);
    can_frame_t wrong_sequence = {OBD_RESPONSE_ECU1, 8, {0x22, 0, 0, 0, 0, 0, 0, 0}};
    isotp_add_frame(&r, &first);
    CHECK(!isotp_add_frame(&r, &wrong_sequence), "a consecutive frame out of sequence was accepted");
}
