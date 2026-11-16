// Host-side tests for everything under src/app: no hardware, built with
// the address and undefined-behavior sanitizers.

#include "check.h"

int failures;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    test_obd_decode();
    test_gt86_decode();
    test_elm_parsing();
    test_elm_session();
    test_elm_init_rejected();
    test_elm_oil();
    test_elm_oil_car_off();
    test_elm_scan();
    test_elm_multi_pid();
    test_elm_listen();
    test_isotp();
    test_flashlog();
    test_settings();
    test_screen();
    test_sensors();
    test_extremes();
    test_off_throttle();
    test_datalog();
    test_wrap();
    test_drive_mode();
    test_elm_drive_mode();
    test_choice_and_idle();
    test_mcp_check();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
