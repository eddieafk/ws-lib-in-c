#include "websocket.h"

typedef struct ws_event_listener {
    char *event_type;
    void (*callback)(ws_client_t *client, void *data);
    void *user_data;
    struct ws_event_listener *next;
} ws_event_listener_t;

typedef struct ws_event_target_impl {
    ws_event_listener_t *listeners;
    pthread_mutex_t mutex;
} ws_event_target_impl_t;

ws_event_target_impl_t* ws_event_target_create(void) {
    ws_event_target_impl_t *target = malloc(sizeof(ws_event_target_impl_t));
    if (!target) return NULL;

    target->listeners = NULL;
    pthread_mutex_init(&target->mutex, NULL);

    return target;
}

void ws_event_target_destroy(ws_event_target_impl_t *target) {
    if (!target) return;

    pthread_mutex_lock(&target->mutex);

    ws_event_listener_t *listener = target->listeners;
    while (listener) {
        ws_event_listener_t *next = listener->next;
        free(listener->event_type);
        free(listener);
        listener = next;
    }

    pthread_mutex_unlock(&target->mutex);
    pthread_mutex_destroy(&target->mutex);
    free(target);
}

int ws_event_target_add_listener(ws_event_target_impl_t *target, const char *event_type,
                                 void (*callback)(ws_client_t*, void*), void *user_data) {
    if (!target || !event_type || !callback) return -1;

    ws_event_listener_t *listener = malloc(sizeof(ws_event_listener_t));
    if (!listener) return -1;

    listener->event_type = strdup(event_type);
    listener->callback = callback;
    listener->user_data = user_data;

    pthread_mutex_lock(&target->mutex);
    listener->next = target->listeners;
    target->listeners = listener;
    pthread_mutex_unlock(&target->mutex);

    return 0;
                                 }

                                 void ws_event_target_dispatch(ws_event_target_impl_t *target, const char *event_type,
                                                               ws_client_t *client, void *data) {
                                     if (!target || !event_type) return;

                                     pthread_mutex_lock(&target->mutex);

                                     ws_event_listener_t *listener = target->listeners;
                                     while (listener) {
                                         if (strcmp(listener->event_type, event_type) == 0) {
                                             listener->callback(client, data);
                                         }
                                         listener = listener->next;
                                     }

                                     pthread_mutex_unlock(&target->mutex);
                                                               }
