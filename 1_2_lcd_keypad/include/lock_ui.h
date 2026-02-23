/**
 * @file lock_ui.h
 * @brief UI/display helper functions for lock system
 */

#ifndef LOCK_UI_H
#define LOCK_UI_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Display centered text on LCD row
 *
 * @param text Text to center (max 16 chars)
 * @param row Row number (0-1)
 */
void lock_ui_display_centered(const char *text, uint8_t row);

/**
 * @brief Display title on row 0 (centered)
 *
 * @param title Title text to display
 */
void lock_ui_display_title(const char *title);

/**
 * @brief Display error message on row 1 with LED/buzzer feedback
 *
 * @param message Error message to display
 * @param delay_ms Delay in milliseconds before returning
 */
void lock_ui_display_error(const char *message, uint32_t delay_ms);

/**
 * @brief Display success message on row 1 with LED/buzzer feedback
 *
 * @param message Success message to display
 * @param delay_ms Delay in milliseconds before returning
 */
void lock_ui_display_success(const char *message, uint32_t delay_ms);

#endif // LOCK_UI_H
