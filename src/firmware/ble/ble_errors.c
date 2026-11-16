// What BTstack's and the controller's error codes mean, in a few words
// that fit the panel: the failure screen says what to do about it.

#include <stdio.h>

#include "ble_internal.h"

static const char *meaning(uint8_t code) {
    switch (code) {
    case ERROR_CODE_UNKNOWN_CONNECTION_IDENTIFIER:
        return "no such connection";
    case ERROR_CODE_CONNECTION_TIMEOUT:
        return "the dongle went out of range";
    case ERROR_CODE_COMMAND_DISALLOWED:
        return "a connection is already starting";
    case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION:
        return "the dongle hung up";
    case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST:
        return "closed by the dash";
    case ERROR_CODE_LMP_RESPONSE_TIMEOUT_LL_RESPONSE_TIMEOUT:
        return "the dongle stopped answering";
    case ERROR_CODE_UNACCEPTABLE_CONNECTION_PARAMETERS:
        return "the dongle refused the link's settings";
    case ERROR_CODE_CONNECTION_FAILED_TO_BE_ESTABLISHED:
        return "the link never came up";
    case BTSTACK_BUSY:
        return "the radio is busy";
    case BTSTACK_MEMORY_ALLOC_FAILED:
        return "the radio still holds another connection";
    case BTSTACK_ACL_BUFFERS_FULL:
        return "the radio's buffers are full";
    default:
        return NULL;
    }
}

void ble_error(char *out, size_t size, const char *what, uint8_t code) {
    const char *text = meaning(code);
    if (text) {
        snprintf(out, size, "%s: %s (0x%02x)", what, text, code);
    } else {
        snprintf(out, size, "%s (HCI 0x%02x)", what, code);
    }
}
