#include "store.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hardware/flash.h"
#include "hardware/structs/psm.h"
#include "hardware/watchdog.h"
#include "pico/flash.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "store/flashlog.h"

#define BTSTACK_SIZE (2 * FLASH_SECTOR_SIZE)
#define LOG_SIZE (64 * 1024)
#define LOG_OFFSET (PICO_FLASH_SIZE_BYTES - BTSTACK_SIZE - LOG_SIZE)
#define SETTINGS_OFFSET (LOG_OFFSET - FLASH_SECTOR_SIZE)
// The data log takes everything from the first megabyte up: the
// firmware is about half of that.
#define DATALOG_OFFSET STORE_FIRMWARE_ROOM
#define DATALOG_SIZE (SETTINGS_OFFSET - DATALOG_OFFSET)

extern char __flash_binary_end;

static flashlog_t flash_log;
static bool log_ready;
static datalog_t data_log;
static bool datalog_ready;

typedef struct {
    uint32_t offset;
    const uint8_t *page;
} program_args_t;

static void erase_sector_now(void *param) {
    flash_range_erase((uint32_t)(uintptr_t)param, FLASH_SECTOR_SIZE);
}

static void program_page_now(void *param) {
    const program_args_t *args = param;
    flash_range_program(args->offset, args->page, FLASH_PAGE_SIZE);
}

// Every flash write, ours and BTstack's (its keys, through the SDK's
// pico_btstack_flash_bank), goes through flash_safe_execute, which asks
// this helper to make the flash safe: interrupts off on core0, and core1,
// once it reads the CAN bus (mcp2515.c) from flash, held in RAM meanwhile
// (multicore lockout). The SDK's own helper can't: with its lockout on it
// refuses every write while core1 isn't running, so it's off
// (PICO_FLASH_SAFE_EXECUTE_PICO_SUPPORT_MULTICORE_LOCKOUT=0 in
// CMakeLists.txt) and this one replaces it. A lockout that times out
// (core1 crashed, a panic) writes anyway. Core0 alone writes flash.
#define LOCKOUT_US 500000
static bool core1_locked;
static uint32_t irqs;

static bool safety_init_deinit(bool init) {
    (void)init;
    return true;
}

static int safety_enter(uint32_t timeout_ms) {
    (void)timeout_ms;
    core1_locked = multicore_lockout_victim_is_initialized(1) && multicore_lockout_start_timeout_us(LOCKOUT_US);
    irqs = save_and_disable_interrupts();
    return PICO_OK;
}

static int safety_exit(uint32_t timeout_ms) {
    (void)timeout_ms;
    restore_interrupts(irqs);
    if (core1_locked) {
        multicore_lockout_end_timeout_us(LOCKOUT_US);
    }
    return PICO_OK;
}

flash_safety_helper_t *get_flash_safety_helper(void) {
    static flash_safety_helper_t helper = {safety_init_deinit, safety_enter, safety_exit};
    return &helper;
}

static void flash_op(void (*op)(void *), void *param) {
    flash_safe_execute(op, param, UINT32_MAX);
}

// An erase feeds the watchdog: erasing a staged update's sectors takes
// longer than it allows.
static void erase_sector(uint32_t offset) {
    flash_op(erase_sector_now, (void *)(uintptr_t)offset);
    watchdog_update();
}

static void program_page(uint32_t offset, const uint8_t *page) {
    program_args_t args = {offset, page};
    flash_op(program_page_now, &args);
}

static void read_log_region(void *ctx, uint32_t offset, uint8_t *out, size_t len) {
    (void)ctx;
    memcpy(out, (const void *)(XIP_BASE + LOG_OFFSET + offset), len);
}

static void program_log_region(void *ctx, uint32_t offset, const uint8_t page[FLASHLOG_PAGE]) {
    (void)ctx;
    program_page(LOG_OFFSET + offset, page);
}

static void erase_log_region(void *ctx, uint32_t offset) {
    (void)ctx;
    erase_sector(LOG_OFFSET + offset);
}

// The data log's region, its offsets as the log's: ctx is its base.
static void read_datalog_region(void *ctx, uint32_t offset, uint8_t *out, size_t len) {
    memcpy(out, (const void *)(XIP_BASE + (uintptr_t)ctx + offset), len);
}

static void program_datalog_region(void *ctx, uint32_t offset, const uint8_t page[FLASHLOG_PAGE]) {
    program_page((uint32_t)(uintptr_t)ctx + offset, page);
}

static void erase_datalog_region(void *ctx, uint32_t offset) {
    erase_sector((uint32_t)(uintptr_t)ctx + offset);
}

void store_init(void) {
    // Never erase over the program: the settings sector is the lowest one.
    if ((uintptr_t)&__flash_binary_end - XIP_BASE > SETTINGS_OFFSET) {
        printf("store: the firmware reaches the settings sector, flash storage disabled\n");
        return;
    }
    flash_dev_t dev = {NULL, LOG_SIZE, read_log_region, program_log_region, erase_log_region};
    log_ready = flashlog_open(&flash_log, dev);
    if ((uintptr_t)&__flash_binary_end - XIP_BASE > DATALOG_OFFSET) {
        log_line("store: the firmware reaches the data log, data logging off");
        return;
    }
    flash_dev_t datalog_dev = {(void *)DATALOG_OFFSET, DATALOG_SIZE, read_datalog_region, program_datalog_region,
                               erase_datalog_region};
    // An update's image left in the ring: installed (this firmware is
    // it), or cut short. Either way it goes before the log reads it.
    update_t staged;
    if (update_find(&staged, datalog_dev)) {
        bool installed = update_crc32(0, (const uint8_t *)XIP_BASE, staged.size) == staged.crc;
        log_line(installed ? "boot: firmware updated from the phone" : "boot: an update cut short, its image erased");
        update_erase(&staged, true);
    }
    datalog_open(&data_log, datalog_dev);
    datalog_ready = true;
    log_line("datalog: session %u", data_log.session);
}

datalog_t *store_get_datalog(void) {
    return datalog_ready ? &data_log : NULL;
}

void log_line(const char *fmt, ...) {
    char line[200];
    uint32_t ms = to_ms_since_boot(get_absolute_time());
    int n = snprintf(line, sizeof line, "+%lu.%lus ", (unsigned long)(ms / 1000), (unsigned long)(ms / 100 % 10));
    va_list args;
    va_start(args, fmt);
    vsnprintf(line + n, sizeof line - (size_t)n, fmt, args);
    va_end(args);
    printf("%s\n", line);
    if (log_ready) {
        flashlog_append(&flash_log, line);
    }
}

static void write_to_stdout(void *ctx, const uint8_t *data, size_t len) {
    (void)ctx;
    fwrite(data, 1, len, stdout);
}

void store_dump_log_on_request(void) {
    if (getchar_timeout_us(0) != 'l' || !log_ready) {
        return;
    }
    printf("--- flash log ---\n");
    flashlog_dump(&flash_log, write_to_stdout, NULL);
    printf("--- end of log ---\n");
    fflush(stdout);
}

settings_t store_load_settings(void) {
    return settings_decode((const uint8_t *)(XIP_BASE + SETTINGS_OFFSET));
}

void store_save_settings(settings_t cfg) {
    if (!log_ready) {
        return;
    }
    uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof page);
    settings_encode(cfg, page);
    erase_sector(SETTINGS_OFFSET);
    program_page(SETTINGS_OFFSET, page);
    log_line("settings: saved");
}

uint32_t store_firmware_crc(uint32_t *size) {
    static uint32_t crc;
    static bool known;
    *size = (uint32_t)((uintptr_t)&__flash_binary_end - XIP_BASE);
    if (!known) {
        crc = update_crc32(0, (const uint8_t *)XIP_BASE, *size);
        known = true;
    }
    return crc;
}

// The copy runs from RAM, one page at a time through it: the flash can't
// be read while it's written. It calls nothing in flash: the SDK's erase
// and program are in RAM, the loop copies words itself (a copy loop
// compiled into a memcpy call would be in flash).
static uint32_t install_page[FLASH_PAGE_SIZE / 4];

static void __attribute__((noreturn)) __no_inline_not_in_flash_func(copy_image)(uint32_t from, uint32_t ring_start,
                                                                                uint32_t ring_end, uint32_t sectors) {
    for (uint32_t i = 0; i < sectors; i++) {
        flash_range_erase(i * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
        for (uint32_t page = 0; page < FLASH_SECTOR_SIZE; page += FLASH_PAGE_SIZE) {
            const volatile uint32_t *source = (const volatile uint32_t *)(XIP_BASE + from + page);
            for (uint32_t word = 0; word < FLASH_PAGE_SIZE / 4; word++) {
                install_page[word] = source[word];
            }
            flash_range_program(i * FLASH_SECTOR_SIZE + page, (const uint8_t *)install_page, FLASH_PAGE_SIZE);
        }
        from += FLASH_SECTOR_SIZE;
        if (from >= ring_end) {
            from = ring_start;
        }
    }
    watchdog_hw->ctrl = WATCHDOG_CTRL_TRIGGER_BITS;
    for (;;) {
    }
}

void store_install(const update_t *u) {
    log_line("update: installing %lu bytes", (unsigned long)u->size);
    uint32_t sectors = (u->size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
    // core1 runs from flash: stopped. Nothing else runs from here on, the
    // watchdog off (the copy takes about as long as it allows); its reset
    // at the end reaches everything but the oscillators, as the SDK's.
    multicore_reset_core1();
    hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
    hw_set_bits(&psm_hw->wdsel, PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS));
    watchdog_hw->scratch[4] = 0;
    save_and_disable_interrupts();
    copy_image(DATALOG_OFFSET + update_image_offset(u, 0), DATALOG_OFFSET, DATALOG_OFFSET + DATALOG_SIZE, sectors);
}
