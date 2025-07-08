#include "websocket.h"

int ws_parse_frame(const uint8_t *data, size_t length, ws_frame_t *frame) {
    if (length < 2) return -1; // Minimum frame size

    size_t pos = 0;

    // First byte: FIN, RSV1-3, Opcode
    uint8_t first_byte = data[pos++];
    frame->fin = (first_byte & 0x80) >> 7;
    frame->rsv1 = (first_byte & 0x40) >> 6;
    frame->rsv2 = (first_byte & 0x20) >> 5;
    frame->rsv3 = (first_byte & 0x10) >> 4;
    frame->opcode = first_byte & 0x0F;

    // Second byte: MASK, Payload length
    uint8_t second_byte = data[pos++];
    frame->mask = (second_byte & 0x80) >> 7;
    uint8_t payload_len = second_byte & 0x7F;

    // Extended payload length
    if (payload_len == 126) {
        if (length < pos + 2) return -1;
        frame->payload_length = (data[pos] << 8) | data[pos + 1];
        pos += 2;
    } else if (payload_len == 127) {
        if (length < pos + 8) return -1;
        frame->payload_length = 0;
        for (int i = 0; i < 8; i++) {
            frame->payload_length = (frame->payload_length << 8) | data[pos + i];
        }
        pos += 8;
    } else {
        frame->payload_length = payload_len;
    }

    // Masking key
    if (frame->mask) {
        if (length < pos + 4) return -1;
        memcpy(frame->masking_key, data + pos, 4);
        pos += 4;
    }

    // Payload data
    if (length < pos + frame->payload_length) return -1;

    frame->payload = malloc(frame->payload_length);
    if (!frame->payload) return -1;

    memcpy(frame->payload, data + pos, frame->payload_length);

    // Apply mask if present
    if (frame->mask) {
        ws_apply_mask(frame->payload, frame->payload_length, frame->masking_key);
    }

    return pos + frame->payload_length;
}
