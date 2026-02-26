#include "button.h"

void button_init(button_state_t *s, bool initial_level) {
    s->stable_level = initial_level;
    s->debounce_count = 0;
    s->last_pressed = (initial_level == APP_BUTTON_ACTIVE_LEVEL);
}

void button_poll(button_state_t *s, button_event_t *ev, bool raw_level) {
    // Debounce: require APP_DEBOUNCE_TICKS consecutive samples
    if (raw_level == s->stable_level) {
        s->debounce_count = 0; // stable
    } else {
        if (s->debounce_count < APP_DEBOUNCE_TICKS) {
            s->debounce_count++;
        }
        if (s->debounce_count >= APP_DEBOUNCE_TICKS) {
            s->stable_level = raw_level;
            s->debounce_count = 0;
        }
    }

    bool pressed = (s->stable_level == APP_BUTTON_ACTIVE_LEVEL);
    bool prev_pressed = s->last_pressed;
    ev->pressed_event = (!prev_pressed && pressed);
    ev->released_event = (prev_pressed && !pressed);
    ev->pressed = pressed;
    s->last_pressed = pressed;
}
