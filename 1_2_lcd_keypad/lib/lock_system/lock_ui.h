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
 * @param text Text to center (max 20 chars)
 * @param row Row number (0-3)
 */
void lock_ui_display_centered(const char *text, uint8_t row);

/**
 * @brief Clear LCD and display a title bar
 * 
 * @param title Title text to display centered
 */
void lock_ui_display_title(const char *title);

/**
 * @brief Display error message with standard formatting
 * 
 * @param message Error message to display
 * @param delay_ms Delay in milliseconds before returning
 */
void lock_ui_display_error(const char *message, uint32_t delay_ms);

/**
 * @brief Display success message with standard formatting
 * 
 * @param message Success message to display
 * @param delay_ms Delay in milliseconds before returning
 */
void lock_ui_display_success(const char *message, uint32_t delay_ms);

/**
 * @brief Clear content area (rows 2-3) without clearing title
 * 
 * Use this instead of lcd_clear() to reduce flicker during state transitions.
 */
void lock_ui_clear_content(void);

/**
 * @brief Clear a specific LCD row
 * 
 * @param row Row number (0-3) to clear
 */
void lock_ui_clear_row(uint8_t row);

#endif // LOCK_UI_H
