// Host-side tests for everything under src/app: no hardware, built with
// the address and undefined-behavior sanitizers.

#include "check.h"

int failures;

int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    test_obd_decode();
    test_gt86_decode();
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
    test_choice_and_idle();
    test_mcp_check();
    test_limiter_flash();
    test_gear_and_trip();
    test_orientations();
    test_doors();
    test_live();
    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("all tests passed\n");
    return 0;
}
