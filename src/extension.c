#include "websocket.h"

typedef struct ws_extension {
    char *name;
    int (*negotiate)(const char *client_extensions, char *server_response);
    int (*encode)(const uint8_t *input, size_t input_len, uint8_t **output, size_t *output_len);
    int (*decode)(const uint8_t *input, size_t input_len, uint8_t **output, size_t *output_len);
    void *context;
} ws_extension_t;

static ws_extension_t *extensions = NULL;
static int extension_count = 0;

int ws_extension_register(const char *name,
                          int (*negotiate)(const char*, char*),
                          int (*encode)(const uint8_t*, size_t, uint8_t**, size_t*),
                          int (*decode)(const uint8_t*, size_t, uint8_t**, size_t*)) {
    extensions = realloc(extensions, (extension_count + 1) * sizeof(ws_extension_t));
    if (!extensions) return -1;

    ws_extension_t *ext = &extensions[extension_count];
    ext->name = strdup(name);
    ext->negotiate = negotiate;
    ext->encode = encode;
    ext->decode = decode;
    ext->context = NULL;

    extension_count++;
    return 0;
                          }

                          int ws_extension_negotiate(const char *client_extensions, char *server_response) {
                              if (!client_extensions || !server_response) return -1;

                              server_response[0] = '\0';

                              for (int i = 0; i < extension_count; i++) {
                                  if (strstr(client_extensions, extensions[i].name)) {
                                      if (extensions[i].negotiate) {
                                          if (extensions[i].negotiate(client_extensions, server_response) == 0) {
                                              return 0; // Extension negotiated
                                          }
                                      }
                                  }
                              }

                              return -1; // No extensions negotiated
                          }
