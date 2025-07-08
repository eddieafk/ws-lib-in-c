#include "websocket.h"

void on_connection(ws_client_t *client) {
    printf("Client connected from %s:%d\n",
           inet_ntoa(client->address.sin_addr),
           ntohs(client->address.sin_port));

    // Send welcome message
    ws_send_text(client->socket, "Welcome to WebSocket server!");
}

void on_message(ws_client_t *client, const char *message, size_t length, ws_opcode_t opcode) {
    printf("Received message from client: ");

    if (opcode == WS_TEXT) {
        printf("TEXT: %.*s\n", (int)length, message);

        // Echo the message back
        char response[1024];
        snprintf(response, sizeof(response), "Echo: %.*s", (int)length, message);
        ws_send_text(client->socket, response);
    } else if (opcode == WS_BINARY) {
        printf("BINARY: %zu bytes\n", length);

        // Echo binary data back
        ws_send_binary(client->socket, (uint8_t*)message, length);
    }
}

void on_close(ws_client_t *client) {
    printf("Client disconnected from %s:%d\n",
           inet_ntoa(client->address.sin_addr),
           ntohs(client->address.sin_port));
}

void on_error(ws_client_t *client, const char *error) {
    printf("Error with client %s:%d: %s\n",
           inet_ntoa(client->address.sin_addr),
           ntohs(client->address.sin_port),
           error);
}

int main(int argc, char *argv[]) {
    int port = 8080;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // Create WebSocket server
    ws_server_t *server = ws_server_create(port);
    if (!server) {
        fprintf(stderr, "Failed to create WebSocket server\n");
        return 1;
    }

    // Set event handlers
    ws_event_target_t event_target = {
        .on_connection = on_connection,
        .on_message = on_message,
        .on_close = on_close,
        .on_error = on_error
    };

    ws_server_set_event_target(server, &event_target);

    // Start server
    if (ws_server_start(server) != 0) {
        fprintf(stderr, "Failed to start WebSocket server\n");
        ws_server_destroy(server);
        return 1;
    }

    printf("WebSocket server started on port %d\n", port);
    printf("Press Ctrl+C to stop the server\n");

    // Keep server running
    while (1) {
        sleep(1);
    }

    // Cleanup
    ws_server_destroy(server);
    return 0;
}
