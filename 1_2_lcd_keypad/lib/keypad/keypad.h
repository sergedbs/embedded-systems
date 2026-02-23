#ifndef KEYPAD_H
#define KEYPAD_H

#include "esp_err.h"

/**
 * @brief Initialize 4x4 matrix keypad
 *
 * Configures rows as outputs and columns as inputs with pullups.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t keypad_init(void);

/**
 * @brief Non-blocking key scan
 *
 * @return Key character, or '\0' if no key is pressed
 */
char keypad_getkey(void);

/**
 * @brief Blocking key read with debounce
 *
 * Waits until a key is pressed and released.
 *
 * @return Key character
 */
char keypad_getkey_blocking(void);

#endif // KEYPAD_H
