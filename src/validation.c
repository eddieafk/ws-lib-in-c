#include "websocket.h"

int ws_validate_utf8(const uint8_t *data, size_t length) {
    size_t i = 0;
    while (i < length) {
        uint8_t byte = data[i];
        int bytes_to_check = 0;

        // ASCII (0xxxxxxx)
        if ((byte & 0x80) == 0) {
            bytes_to_check = 1;
        }
        // 2-byte sequence (110xxxxx 10xxxxxx)
        else if ((byte & 0xE0) == 0xC0) {
            bytes_to_check = 2;
        }
        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        else if ((byte & 0xF0) == 0xE0) {
            bytes_to_check = 3;
        }
        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        else if ((byte & 0xF8) == 0xF0) {
            bytes_to_check = 4;
        }
        else {
            return 0; // Invalid start byte
        }

        if (i + bytes_to_check > length) {
            return 0; // Not enough bytes
        }

        // Check continuation bytes
        for (int j = 1; j < bytes_to_check; j++) {
            if ((data[i + j] & 0xC0) != 0x80) {
                return 0; // Invalid continuation byte
            }
        }

        i += bytes_to_check;
    }
    return 1; // Valid UTF-8
}
