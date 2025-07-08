#include "websocket.h"

typedef struct ws_subprotocol {
    char *name;
    int (*validate)(const char *subprotocol);
    void (*on_message)(ws_client_t *client, const char *message, size_t length);
} ws_subprotocol_t;

static ws_subprotocol_t *subprotocols = NULL;
static int subprotocol_count = 0;

int ws_subprotocol_register(const char *name,
                            int (*validate)(const char*),
                            void (*on_message)(ws_client_t*, const char*, size_t)) {
    subprotocols = realloc(subprotocols, (subprotocol_count + 1) * sizeof(ws_subprotocol_t));
    if (!subprotocols) return -1;

    ws_subprotocol_t *proto = &subprotocols[subprotocol_count];
    proto->name = strdup(name);
    proto->validate = validate;
    proto->on_message = on_message;

    subprotocol_count++;
    return 0;
                            }

                            const char* ws_subprotocol_select(const char *client_protocols) {
                                if (!client_protocols) return NULL;

                                char *protocols = strdup(client_protocols);
                                char *token = strtok(protocols, ",");

                                while (token) {
                                    // Trim whitespace
                                    while (*token == ' ') token++;
                                    char *end = token + strlen(token) - 1;
                                    while (end > token && *end == ' ') end--;
                                    *(end + 1) = '\0';

                                    // Check if we support this protocol
                                    for (int i = 0; i < subprotocol_count; i++) {
                                        if (strcmp(token, subprotocols[i].name) == 0) {
                                            if (!subprotocols[i].validate || subprotocols[i].validate(token)) {
                                                free(protocols);
                                                return subprotocols[i].name;
                                            }
                                        }
                                    }

                                    token = strtok(NULL, ",");
                                }

                                free(protocols);
                                return NULL;
                            }
