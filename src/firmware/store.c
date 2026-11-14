#include "store.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "flashlog.h"
#include "hardware/flash.h"
#include "pico/flash.h"
#include "pico/stdlib.h"

#define BTSTACK_SIZE (2 * FLASH_SECTOR_SIZE)
#define LOG_SIZE (64 * 1024)
#define LOG_OFFSET (PICO_FLASH_SIZE_BYTES - BTSTACK_SIZE - LOG_SIZE)
#define SETTINGS_OFFSET (LOG_OFFSET - FLASH_SECTOR_SIZE)

extern char __flash_binary_end;

static flashlog_t flash_log;
static bool log_ready;

typedef struct {
    uint32_t offset;
    const uint8_t *page;
} program_args_t;

static void do_erase(void *param) {
    flash_range_erase((uint32_t)(uintptr_t)param, FLASH_SECTOR_SIZE);
}

static void do_program(void *param) {
    const program_args_t *args = param;
    flash_range_program(args->offset, args->page, FLASH_PAGE_SIZE);
}

// flash_safe_execute runs with interrupts off, the code it calls from RAM.
static void erase_sector(uint32_t offset) {
    flash_safe_execute(do_erase, (void *)(uintptr_t)offset, UINT32_MAX);
}

static void program_page(uint32_t offset, const uint8_t *page) {
    program_args_t args = {offset, page};
    flash_safe_execute(do_program, &args, UINT32_MAX);
}

static void log_read(void *ctx, uint32_t offset, uint8_t *out, size_t len) {
    (void)ctx;
    memcpy(out, (const void *)(XIP_BASE + LOG_OFFSET + offset), len);
}

static void log_program(void *ctx, uint32_t offset, const uint8_t page[FLASHLOG_PAGE]) {
    (void)ctx;
    program_page(LOG_OFFSET + offset, page);
}

static void log_erase(void *ctx, uint32_t offset) {
    (void)ctx;
    erase_sector(LOG_OFFSET + offset);
}

void store_init(void) {
    // Never erase over the program: the settings sector is the lowest one.
    if ((uintptr_t)&__flash_binary_end - XIP_BASE > SETTINGS_OFFSET) {
        printf("store: the firmware reaches the settings sector, flash storage disabled\n");
        return;
    }
    flash_dev_t dev = {NULL, LOG_SIZE, log_read, log_program, log_erase};
    log_ready = flashlog_open(&flash_log, dev);
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

static void write_stdout(void *ctx, const uint8_t *data, size_t len) {
    (void)ctx;
    fwrite(data, 1, len, stdout);
}

void store_serve_log(void) {
    if (getchar_timeout_us(0) != 'l' || !log_ready) {
        return;
    }
    printf("--- flash log ---\n");
    flashlog_dump(&flash_log, write_stdout, NULL);
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
