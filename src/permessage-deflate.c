#include "websocket.h"

ws_compression_t* ws_compression_create(void) {
    ws_compression_t *comp = malloc(sizeof(ws_compression_t));
    if (!comp) return NULL;

    comp->initialized = 0;

    // Initialize deflate stream
    comp->deflate_stream.zalloc = Z_NULL;
    comp->deflate_stream.zfree = Z_NULL;
    comp->deflate_stream.opaque = Z_NULL;

    if (deflateInit2(&comp->deflate_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        free(comp);
        return NULL;
    }

    // Initialize inflate stream
    comp->inflate_stream.zalloc = Z_NULL;
    comp->inflate_stream.zfree = Z_NULL;
    comp->inflate_stream.opaque = Z_NULL;

    if (inflateInit2(&comp->inflate_stream, -15) != Z_OK) {
        deflateEnd(&comp->deflate_stream);
        free(comp);
        return NULL;
    }

    comp->initialized = 1;
    return comp;
}

void ws_compression_destroy(ws_compression_t *comp) {
    if (comp && comp->initialized) {
        deflateEnd(&comp->deflate_stream);
        inflateEnd(&comp->inflate_stream);
        free(comp);
    }
}

int ws_compression_deflate(ws_compression_t *comp, const uint8_t *input, size_t input_len, uint8_t **output, size_t *output_len) {
    if (!comp || !comp->initialized) return -1;

    size_t max_output = deflateBound(&comp->deflate_stream, input_len);
    *output = malloc(max_output);
    if (!*output) return -1;

    comp->deflate_stream.next_in = (Bytef*)input;
    comp->deflate_stream.avail_in = input_len;
    comp->deflate_stream.next_out = *output;
    comp->deflate_stream.avail_out = max_output;

    int result = deflate(&comp->deflate_stream, Z_SYNC_FLUSH);
    if (result != Z_OK) {
        free(*output);
        return -1;
    }

    *output_len = max_output - comp->deflate_stream.avail_out;
    return 0;
}

int ws_compression_inflate(ws_compression_t *comp, const uint8_t *input, size_t input_len, uint8_t **output, size_t *output_len) {
    if (!comp || !comp->initialized) return -1;

    size_t max_output = input_len * 4; // Estimate
    *output = malloc(max_output);
    if (!*output) return -1;

    comp->inflate_stream.next_in = (Bytef*)input;
    comp->inflate_stream.avail_in = input_len;
    comp->inflate_stream.next_out = *output;
    comp->inflate_stream.avail_out = max_output;

    int result = inflate(&comp->inflate_stream, Z_SYNC_FLUSH);
    if (result != Z_OK && result != Z_STREAM_END) {
        free(*output);
        return -1;
    }

    *output_len = max_output - comp->inflate_stream.avail_out;
    return 0;
}
