#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <zlib.h>

// Constants
#define WS_MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_MAGIC_STRING_LEN 36
#define MAX_FRAME_SIZE 65536
#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192

// WebSocket opcodes
typedef enum {
    WS_CONTINUATION = 0x0,
    WS_TEXT = 0x1,
    WS_BINARY = 0x2,
    WS_CLOSE = 0x8,
    WS_PING = 0x9,
    WS_PONG = 0xA
} ws_opcode_t;

// WebSocket frame structure
typedef struct {
    uint8_t fin;
    uint8_t rsv1;
    uint8_t rsv2;
    uint8_t rsv3;
    uint8_t opcode;
    uint8_t mask;
    uint64_t payload_length;
    uint8_t masking_key[4];
    uint8_t *payload;
} ws_frame_t;

// WebSocket client structure
typedef struct {
    int socket;
    int connected;
    char *buffer;
    size_t buffer_size;
    size_t buffer_pos;
    pthread_mutex_t mutex;
    struct sockaddr_in address;
} ws_client_t;

// WebSocket server structure
typedef struct {
    int socket;
    int port;
    int max_clients;
    ws_client_t *clients;
    pthread_mutex_t clients_mutex;
    int running;
    pthread_t server_thread;
} ws_server_t;

// Event target structure
typedef struct ws_event_target {
    void (*on_connection)(ws_client_t *client);
    void (*on_message)(ws_client_t *client, const char *message, size_t length, ws_opcode_t opcode);
    void (*on_close)(ws_client_t *client);
    void (*on_error)(ws_client_t *client, const char *error);
} ws_event_target_t;

// Function declarations
ws_server_t* ws_server_create(int port);
int ws_server_start(ws_server_t *server);
void ws_server_stop(ws_server_t *server);
void ws_server_destroy(ws_server_t *server);
void ws_server_set_event_target(ws_server_t *server, ws_event_target_t *target);

int ws_handshake(int client_socket);
int ws_parse_frame(const uint8_t *data, size_t length, ws_frame_t *frame);
int ws_send_frame(int socket, ws_opcode_t opcode, const uint8_t *payload, size_t length);
int ws_send_text(int socket, const char *message);
int ws_send_binary(int socket, const uint8_t *data, size_t length);
int ws_send_ping(int socket, const uint8_t *data, size_t length);
int ws_send_pong(int socket, const uint8_t *data, size_t length);
int ws_send_close(int socket, uint16_t code, const char *reason);

// Utility functions
char* ws_base64_encode(const uint8_t *data, size_t length);
int ws_base64_decode(const char *input, uint8_t *output, size_t *output_length);
void ws_generate_accept_key(const char *client_key, char *accept_key);
int ws_validate_utf8(const uint8_t *data, size_t length);
void ws_apply_mask(uint8_t *data, size_t length, const uint8_t *mask);

// Buffer utilities
typedef struct {
    uint8_t *data;
    size_t size;
    size_t capacity;
} ws_buffer_t;

ws_buffer_t* ws_buffer_create(size_t initial_capacity);
void ws_buffer_destroy(ws_buffer_t *buffer);
int ws_buffer_append(ws_buffer_t *buffer, const uint8_t *data, size_t length);
void ws_buffer_clear(ws_buffer_t *buffer);

// Compression support (permessage-deflate)
typedef struct {
    z_stream deflate_stream;
    z_stream inflate_stream;
    int initialized;
} ws_compression_t;

ws_compression_t* ws_compression_create(void);
void ws_compression_destroy(ws_compression_t *comp);
int ws_compression_deflate(ws_compression_t *comp, const uint8_t *input, size_t input_len, uint8_t **output, size_t *output_len);
int ws_compression_inflate(ws_compression_t *comp, const uint8_t *input, size_t input_len, uint8_t **output, size_t *output_len);

// Rate limiter
typedef struct {
    time_t last_reset;
    int count;
    int max_requests;
    int window_seconds;
} ws_rate_limiter_t;

ws_rate_limiter_t* ws_rate_limiter_create(int max_requests, int window_seconds);
void ws_rate_limiter_destroy(ws_rate_limiter_t *limiter);
int ws_rate_limiter_check(ws_rate_limiter_t *limiter);

#endif
