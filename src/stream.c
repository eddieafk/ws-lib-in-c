#include "websocket.h"

typedef struct ws_stream {
    int socket;
    ws_buffer_t *read_buffer;
    ws_buffer_t *write_buffer;
    pthread_mutex_t read_mutex;
    pthread_mutex_t write_mutex;
    int non_blocking;
} ws_stream_t;

ws_stream_t* ws_stream_create(int socket) {
    ws_stream_t *stream = malloc(sizeof(ws_stream_t));
    if (!stream) return NULL;

    stream->socket = socket;
    stream->read_buffer = ws_buffer_create(BUFFER_SIZE);
    stream->write_buffer = ws_buffer_create(BUFFER_SIZE);
    stream->non_blocking = 0;

    if (!stream->read_buffer || !stream->write_buffer) {
        ws_buffer_destroy(stream->read_buffer);
        ws_buffer_destroy(stream->write_buffer);
        free(stream);
        return NULL;
    }

    pthread_mutex_init(&stream->read_mutex, NULL);
    pthread_mutex_init(&stream->write_mutex, NULL);

    return stream;
}

void ws_stream_destroy(ws_stream_t *stream) {
    if (stream) {
        ws_buffer_destroy(stream->read_buffer);
        ws_buffer_destroy(stream->write_buffer);
        pthread_mutex_destroy(&stream->read_mutex);
        pthread_mutex_destroy(&stream->write_mutex);
        free(stream);
    }
}

int ws_stream_set_non_blocking(ws_stream_t *stream, int non_blocking) {
    if (!stream) return -1;

    int flags = fcntl(stream->socket, F_GETFL, 0);
    if (flags == -1) return -1;

    if (non_blocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    stream->non_blocking = non_blocking;
    return fcntl(stream->socket, F_SETFL, flags);
}

int ws_stream_read(ws_stream_t *stream, uint8_t *buffer, size_t size) {
    if (!stream) return -1;

    pthread_mutex_lock(&stream->read_mutex);

    // Try to read from internal buffer first
    if (stream->read_buffer->size > 0) {
        size_t to_copy = (size < stream->read_buffer->size) ? size : stream->read_buffer->size;
        memcpy(buffer, stream->read_buffer->data, to_copy);

        // Shift remaining data
        memmove(stream->read_buffer->data, stream->read_buffer->data + to_copy,
                stream->read_buffer->size - to_copy);
        stream->read_buffer->size -= to_copy;

        pthread_mutex_unlock(&stream->read_mutex);
        return to_copy;
    }

    // Read from socket
    int result = recv(stream->socket, buffer, size, 0);
    pthread_mutex_unlock(&stream->read_mutex);

    return result;
}

int ws_stream_write(ws_stream_t *stream, const uint8_t *buffer, size_t size) {
    if (!stream) return -1;

    pthread_mutex_lock(&stream->write_mutex);

    // Try to send directly first
    int sent = send(stream->socket, buffer, size, MSG_NOSIGNAL);

    if (sent == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Buffer the data
            if (ws_buffer_append(stream->write_buffer, buffer, size) == 0) {
                sent = size; // Indicate success, data buffered
            }
        }
    } else if (sent < size) {
        // Partially sent, buffer remaining
        ws_buffer_append(stream->write_buffer, buffer + sent, size - sent);
    }

    pthread_mutex_unlock(&stream->write_mutex);
    return sent;
}

int ws_stream_flush(ws_stream_t *stream) {
    if (!stream || stream->write_buffer->size == 0) return 0;

    pthread_mutex_lock(&stream->write_mutex);

    int sent = send(stream->socket, stream->write_buffer->data, stream->write_buffer->size, MSG_NOSIGNAL);

    if (sent > 0) {
        // Remove sent data from buffer
        memmove(stream->write_buffer->data, stream->write_buffer->data + sent,
                stream->write_buffer->size - sent);
        stream->write_buffer->size -= sent;
    }

    pthread_mutex_unlock(&stream->write_mutex);
    return sent;
}
