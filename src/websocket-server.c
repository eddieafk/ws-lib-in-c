#include "websocket.h"

static ws_event_target_t *global_event_target = NULL;

ws_server_t* ws_server_create(int port) {
    ws_server_t *server = malloc(sizeof(ws_server_t));
    if (!server) return NULL;

    server->port = port;
    server->max_clients = MAX_CLIENTS;
    server->clients = calloc(MAX_CLIENTS, sizeof(ws_client_t));
    server->running = 0;

    if (pthread_mutex_init(&server->clients_mutex, NULL) != 0) {
        free(server->clients);
        free(server);
        return NULL;
    }

    // Initialize client mutexes
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pthread_mutex_init(&server->clients[i].mutex, NULL);
        server->clients[i].buffer = malloc(BUFFER_SIZE);
        server->clients[i].buffer_size = BUFFER_SIZE;
        server->clients[i].connected = 0;
    }

    return server;
}

void ws_server_set_event_target(ws_server_t *server, ws_event_target_t *target) {
    global_event_target = target;
}

int ws_handshake(int client_socket) {
    char buffer[4096];
    char response[1024];
    char *sec_websocket_key = NULL;
    char accept_key[64];

    // Read HTTP request
    int bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) return -1;

    buffer[bytes_read] = '\0';

    // Parse HTTP headers
    char *line = strtok(buffer, "\r\n");
    while (line) {
        if (strncmp(line, "Sec-WebSocket-Key:", 18) == 0) {
            sec_websocket_key = line + 19;
            while (*sec_websocket_key == ' ') sec_websocket_key++;
            break;
        }
        line = strtok(NULL, "\r\n");
    }

    if (!sec_websocket_key) return -1;

    // Generate accept key
    ws_generate_accept_key(sec_websocket_key, accept_key);

    // Send HTTP response
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             accept_key);

    return send(client_socket, response, strlen(response), 0);
}

void* client_handler(void *arg) {
    ws_client_t *client = (ws_client_t*)arg;
    uint8_t buffer[BUFFER_SIZE];
    ws_frame_t frame;

    // Perform handshake
    if (ws_handshake(client->socket) < 0) {
        close(client->socket);
        client->connected = 0;
        return NULL;
    }

    if (global_event_target && global_event_target->on_connection) {
        global_event_target->on_connection(client);
    }

    while (client->connected) {
        int bytes_received = recv(client->socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }

        // Parse WebSocket frame
        int frame_size = ws_parse_frame(buffer, bytes_received, &frame);
        if (frame_size < 0) {
            if (global_event_target && global_event_target->on_error) {
                global_event_target->on_error(client, "Invalid frame");
            }
            break;
        }

        // Handle different frame types
        switch (frame.opcode) {
            case WS_TEXT:
                if (ws_validate_utf8(frame.payload, frame.payload_length)) {
                    if (global_event_target && global_event_target->on_message) {
                        global_event_target->on_message(client, (char*)frame.payload, frame.payload_length, WS_TEXT);
                    }
                } else {
                    ws_send_close(client->socket, 1007, "Invalid UTF-8");
                    client->connected = 0;
                }
                break;

            case WS_BINARY:
                if (global_event_target && global_event_target->on_message) {
                    global_event_target->on_message(client, (char*)frame.payload, frame.payload_length, WS_BINARY);
                }
                break;

            case WS_PING:
                ws_send_pong(client->socket, frame.payload, frame.payload_length);
                break;

            case WS_PONG:
                // Handle pong frame
                break;

            case WS_CLOSE:
                ws_send_close(client->socket, 1000, "Normal closure");
                client->connected = 0;
                break;
        }

        free(frame.payload);
    }

    if (global_event_target && global_event_target->on_close) {
        global_event_target->on_close(client);
    }

    close(client->socket);
    client->connected = 0;
    return NULL;
}

void* server_thread(void *arg) {
    ws_server_t *server = (ws_server_t*)arg;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create socket
    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket < 0) {
        perror("socket");
        return NULL;
    }

    // Set socket options
    int opt = 1;
    setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server->port);

    if (bind(server->socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server->socket);
        return NULL;
    }

    // Listen for connections
    if (listen(server->socket, 5) < 0) {
        perror("listen");
        close(server->socket);
        return NULL;
    }

    printf("WebSocket server listening on port %d\n", server->port);

    while (server->running) {
        int client_socket = accept(server->socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (server->running) {
                perror("accept");
            }
            continue;
        }

        // Find free client slot
        pthread_mutex_lock(&server->clients_mutex);
        int client_index = -1;
        for (int i = 0; i < server->max_clients; i++) {
            if (!server->clients[i].connected) {
                client_index = i;
                break;
            }
        }

        if (client_index == -1) {
            // No free slots
            close(client_socket);
            pthread_mutex_unlock(&server->clients_mutex);
            continue;
        }

        // Initialize client
        ws_client_t *client = &server->clients[client_index];
        client->socket = client_socket;
        client->connected = 1;
        client->buffer_pos = 0;
        client->address = client_addr;

        pthread_mutex_unlock(&server->clients_mutex);

        // Create thread for client
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_handler, client);
        pthread_detach(client_thread);
    }

    close(server->socket);
    return NULL;
}

int ws_server_start(ws_server_t *server) {
    server->running = 1;
    return pthread_create(&server->server_thread, NULL, server_thread, server);
}

void ws_server_stop(ws_server_t *server) {
    server->running = 0;
    close(server->socket);
    pthread_join(server->server_thread, NULL);
}

void ws_server_destroy(ws_server_t *server) {
    if (server) {
        ws_server_stop(server);

        // Close all client connections
        for (int i = 0; i < server->max_clients; i++) {
            if (server->clients[i].connected) {
                close(server->clients[i].socket);
            }
            free(server->clients[i].buffer);
            pthread_mutex_destroy(&server->clients[i].mutex);
        }

        free(server->clients);
        pthread_mutex_destroy(&server->clients_mutex);
        free(server);
    }
}
