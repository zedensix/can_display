#include "check.h"

void fake_write(void *ctx, const char *command) {
    fake_dongle_t *d = ctx;
    if (d->sent_count < 512) {
        snprintf(d->sent[d->sent_count++], 16, "%s", command);
    }
}
void fake_log(void *ctx, const char *line) {
    fake_dongle_t *d = ctx;
    size_t len = strlen(d->logged);
    snprintf(d->logged + len, sizeof d->logged - len, "%s\n", line);
}
void fake_ready(void *ctx) {
    ((fake_dongle_t *)ctx)->ready++;
}
void fake_failed(void *ctx, const char *step, const char *error) {
    fake_dongle_t *d = ctx;
    d->failed++;
    snprintf(d->failed_step, sizeof d->failed_step, "%s", step);
    (void)error;
}

void feed_dongle_answer(elm_t *e, const char *text, uint32_t now) {
    elm_feed_dongle_output(e, (const uint8_t *)text, strlen(text), now);
}

const char *last_sent(fake_dongle_t *d) {
    return d->sent[d->sent_count - 1];
}
