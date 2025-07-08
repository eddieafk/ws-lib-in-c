#include "websocket.h"

ws_buffer_t* ws_buffer_create(size_t initial_capacity) {
    ws_buffer_t *buffer = malloc(sizeof(ws_buffer_t));
    if (!buffer) return NULL;

    buffer->data = malloc(initial_capacity);
    if (!buffer->data) {
        free(buffer);
        return NULL;
    }

    buffer->size = 0;
    buffer->capacity = initial_capacity;
    return buffer;
}

void ws_buffer_destroy(ws_buffer_t *buffer) {
    if (buffer) {
        free(buffer->data);
        free(buffer);
    }
}

int ws_buffer_append(ws_buffer_t *buffer, const uint8_t *data, size_t length) {
    if (buffer->size + length > buffer->capacity) {
        size_t new_capacity = buffer->capacity * 2;
        while (new_capacity < buffer->size + length) {
            new_capacity *= 2;
        }

        uint8_t *new_data = realloc(buffer->data, new_capacity);
        if (!new_data) return -1;

        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }

    memcpy(buffer->data + buffer->size, data, length);
    buffer->size += length;
    return 0;
}

void ws_buffer_clear(ws_buffer_t *buffer) {
    buffer->size = 0;
}

// Base64 encoding/decoding
char* ws_base64_encode(const uint8_t *data, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data, length);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &buffer_ptr);

    char *result = malloc(buffer_ptr->length + 1);
    memcpy(result, buffer_ptr->data, buffer_ptr->length);
    result[buffer_ptr->length] = '\0';

    BIO_free_all(bio);
    return result;
}

int ws_base64_decode(const char *input, uint8_t *output, size_t *output_length) {
    BIO *bio, *b64;
    int decode_len = strlen(input);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input, decode_len);
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    *output_length = BIO_read(bio, output, decode_len);

    BIO_free_all(bio);
    return (*output_length > 0) ? 0 : -1;
}

// Generate WebSocket accept key
void ws_generate_accept_key(const char *client_key, char *accept_key) {
    char combined[256];
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];

    snprintf(combined, sizeof(combined), "%s%s", client_key, WS_MAGIC_STRING);

    SHA1((unsigned char*)combined, strlen(combined), sha1_hash);

    char *encoded = ws_base64_encode(sha1_hash, SHA_DIGEST_LENGTH);
    strcpy(accept_key, encoded);
    free(encoded);
}

// Apply WebSocket mask
void ws_apply_mask(uint8_t *data, size_t length, const uint8_t *mask) {
    for (size_t i = 0; i < length; i++) {
        data[i] ^= mask[i % 4];
    }
}
