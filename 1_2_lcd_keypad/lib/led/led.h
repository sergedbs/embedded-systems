#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief LED color enumeration
 */
typedef enum {
    LED_GREEN = 0,
    LED_RED = 1
} led_color_t;

/**
 * @brief Initialize LED GPIOs
 * 
 * Configures LED pins as outputs and turns all LEDs off
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t led_init(void);

/**
 * @brief Set LED state
 * 
 * @param color LED color (LED_GREEN or LED_RED)
 * @param state true to turn on, false to turn off
 */
void led_set(led_color_t color, bool state);

/**
 * @brief Turn off all LEDs
 */
void led_all_off(void);

/**
 * @brief Turn on green LED, turn off others
 */
void led_success(void);

/**
 * @brief Turn on red LED, turn off others
 */
void led_error(void);

#endif // LED_H
