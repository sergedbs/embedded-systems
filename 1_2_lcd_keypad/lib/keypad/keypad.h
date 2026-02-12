#ifndef KEYPAD_H
#define KEYPAD_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize 4x4 matrix keypad
 * 
 * Configures rows as outputs and columns as inputs with pullups
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t keypad_init(void);

/**
 * @brief Get key press (non-blocking)
 * 
 * Scans the keypad matrix once and returns the character if a key is pressed.
 * Returns '\0' if no key is pressed.
 * 
 * @return Character from keypad ('0'-'9', 'A'-'D', '*', '#') or '\0' if no key
 */
char keypad_getkey(void);

/**
 * @brief Get key press (blocking)
 * 
 * Waits until a key is pressed and released, with debouncing.
 * This function blocks until a valid keypress is detected.
 * 
 * @return Character from keypad ('0'-'9', 'A'-'D', '*', '#')
 */
char keypad_getkey_blocking(void);

/**
 * @brief Check if any key is currently pressed
 * 
 * @return true if a key is pressed, false otherwise
 */
bool keypad_is_pressed(void);

#endif // KEYPAD_H
