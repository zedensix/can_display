#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elm327.h"

int elm_split_lines(char *raw, const char *command, char *lines[], int max) {
    int n = 0;
    for (char *line = strtok(raw, "\r\n"); line && n < max; line = strtok(NULL, "\r\n")) {
        while (*line == ' ') {
            line++;
        }
        size_t len = strlen(line);
        while (len > 0 && line[len - 1] == ' ') {
            line[--len] = 0;
        }
        if (len == 0 || strcmp(line, command) == 0 || strncmp(line, "SEARCHING", 9) == 0) {
            continue;
        }
        lines[n++] = line;
    }
    return n;
}

static int hex_digit_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

bool elm_parse_frame(const char *line, can_frame_t *f) {
    int digits[3 + 16];
    int n = 0;
    for (; *line; line++) {
        if (*line == ' ') {
            continue;
        }
        int d = hex_digit_value(*line);
        if (d < 0 || n == (int)(sizeof digits / sizeof digits[0])) {
            return false;
        }
        digits[n++] = d;
    }
    int data_digits = n - 3;
    if (data_digits < 2 || data_digits % 2 != 0) {
        return false;
    }
    *f =
        (can_frame_t){.id = (uint32_t)(digits[0] << 8 | digits[1] << 4 | digits[2]), .dlc = (uint8_t)(data_digits / 2)};
    for (int i = 0; i < f->dlc; i++) {
        f->data[i] = (uint8_t)(digits[3 + 2 * i] << 4 | digits[4 + 2 * i]);
    }
    return true;
}

bool elm_parse_voltage(const char *line, float *volts) {
    char *end;
    float v = strtof(line, &end);
    if (end == line || (*end != 'V' && *end != 0)) {
        return false;
    }
    *volts = v;
    return true;
}
