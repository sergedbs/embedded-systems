#ifndef APP_TYPES_H
#define APP_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PRESS_SHORT = 0,
    PRESS_LONG
} press_type_t;

typedef struct {
    press_type_t type;
    uint32_t duration_ms;
} press_event_t;

#define EVENT_QUEUE_SIZE 8

typedef struct {
    press_event_t buf[EVENT_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} event_queue_t;

static inline void event_queue_init(event_queue_t *q) {
    q->head = q->tail = q->count = 0;
}

static inline bool event_queue_is_full(const event_queue_t *q) {
    return q->count == EVENT_QUEUE_SIZE;
}

static inline bool event_queue_is_empty(const event_queue_t *q) {
    return q->count == 0;
}

static inline bool event_queue_push(event_queue_t *q, const press_event_t *ev) {
    if (event_queue_is_full(q)) {
        return false;
    }
    q->buf[q->head] = *ev;
    q->head = (uint8_t)((q->head + 1U) % EVENT_QUEUE_SIZE);
    q->count++;
    return true;
}

static inline bool event_queue_pop(event_queue_t *q, press_event_t *out) {
    if (event_queue_is_empty(q)) {
        return false;
    }
    *out = q->buf[q->tail];
    q->tail = (uint8_t)((q->tail + 1U) % EVENT_QUEUE_SIZE);
    q->count--;
    return true;
}

#endif // APP_TYPES_H
