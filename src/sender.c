#include "websocket.h"

int ws_send_frame(int socket, ws_opcode_t opcode, const uint8_t *payload, size_t length) {
    uint8_t frame[MAX_FRAME_SIZE];
    size_t frame_size = 0;

    // First byte: FIN=1, RSV=0, Opcode
    frame[frame_size++] = 0x80 | (opcode & 0x0F);

    // Second byte and extended length
    if (length < 126) {
        frame[frame_size++] = length;
    } else if (length < 65536) {
        frame[frame_size++] = 126;
        frame[frame_size++] = (length >> 8) & 0xFF;
        frame[frame_size++] = length & 0xFF;
    } else {
        frame[frame_size++] = 127;
        for (int i = 7; i >= 0; i--) {
            frame[frame_size++] = (length >> (i * 8)) & 0xFF;
        }
    }

    // Payload data
    if (payload && length > 0) {
        memcpy(frame + frame_size, payload, length);
        frame_size += length;
    }

    return send(socket, frame, frame_size, 0);
}

int ws_send_text(int socket, const char *message) {
    return ws_send_frame(socket, WS_TEXT, (uint8_t*)message, strlen(message));
}

int ws_send_binary(int socket, const uint8_t *data, size_t length) {
    return ws_send_frame(socket, WS_BINARY, data, length);
}

int ws_send_ping(int socket, const uint8_t *data, size_t length) {
    return ws_send_frame(socket, WS_PING, data, length);
}

int ws_send_pong(int socket, const uint8_t *data, size_t length) {
    return ws_send_frame(socket, WS_PONG, data, length);
}

int ws_send_close(int socket, uint16_t code, const char *reason) {
    uint8_t payload[125];
    size_t payload_len = 0;

    // Close code (2 bytes, big endian)
    payload[payload_len++] = (code >> 8) & 0xFF;
    payload[payload_len++] = code & 0xFF;

    // Reason string
    if (reason) {
        size_t reason_len = strlen(reason);
        if (reason_len > 123) reason_len = 123; // Max reason length
        memcpy(payload + payload_len, reason, reason_len);
        payload_len += reason_len;
    }

    return ws_send_frame(socket, WS_CLOSE, payload, payload_len);
}
