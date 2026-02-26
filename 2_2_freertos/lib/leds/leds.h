#ifndef LEDS_H
#define LEDS_H

#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"

void leds_init(void);
void led_green_set(bool on);
void led_red_set(bool on);
void led_yellow_set(bool on);
bool led_yellow_get(void);

#endif // LEDS_H
