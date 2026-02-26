#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

typedef struct {
    bool stable_level;
    uint8_t debounce_count;
    bool last_pressed;
} button_state_t;

typedef struct {
    bool pressed;           // current debounced logical state
    bool pressed_event;     // rising edge
    bool released_event;    // falling edge
} button_event_t;

void button_init(button_state_t *s, bool initial_level);
void button_poll(button_state_t *s, button_event_t *ev, bool raw_level);

#endif // BUTTON_H
