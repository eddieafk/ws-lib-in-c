#include "websocket.h"

ws_rate_limiter_t* ws_rate_limiter_create(int max_requests, int window_seconds) {
    ws_rate_limiter_t *limiter = malloc(sizeof(ws_rate_limiter_t));
    if (!limiter) return NULL;

    limiter->max_requests = max_requests;
    limiter->window_seconds = window_seconds;
    limiter->count = 0;
    limiter->last_reset = time(NULL);

    return limiter;
}

void ws_rate_limiter_destroy(ws_rate_limiter_t *limiter) {
    if (limiter) {
        free(limiter);
    }
}

int ws_rate_limiter_check(ws_rate_limiter_t *limiter) {
    if (!limiter) return 1; // No limiter, allow

    time_t now = time(NULL);

    // Reset counter if window has passed
    if (now - limiter->last_reset >= limiter->window_seconds) {
        limiter->count = 0;
        limiter->last_reset = now;
    }

    // Check if limit exceeded
    if (limiter->count >= limiter->max_requests) {
        return 0; // Rate limit exceeded
    }

    limiter->count++;
    return 1; // Allow request
}
