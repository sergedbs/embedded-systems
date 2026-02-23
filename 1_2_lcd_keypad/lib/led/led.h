#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief LED color
 */
typedef enum {
    LED_GREEN = 0,
    LED_RED   = 1
} led_color_t;

/**
 * @brief Initialize LED GPIOs (output, initially off)
 */
esp_err_t led_init(void);

/**
 * @brief Set an LED on or off
 */
void led_set(led_color_t color, bool state);

/**
 * @brief Turn off all LEDs
 */
void led_all_off(void);

/**
 * @brief Green ON, red OFF
 */
void led_success(void);

/**
 * @brief Red ON, green OFF
 */
void led_error(void);

#endif // LED_H
