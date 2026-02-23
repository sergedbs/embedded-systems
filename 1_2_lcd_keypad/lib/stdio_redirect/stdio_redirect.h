/**
 * @file stdio_redirect.h
 * @brief STDIO-like wrapper functions for LCD/Keypad
 * 
 * Provides printf/scanf-like functions that work with LCD and Keypad:
 * - lcd_printf() → outputs to LCD display
 * - lcd_scanf() → reads input from keypad
 * 
 * Features:
 * - Printf-style formatting
 * - Backspace handling ('*' key removes last character)
 * - Enter/Confirm ('#' key ends input)
 * - Echo characters to LCD during input
 * 
 * @author ESP32 Lock System
 * @date 2026
 */

#ifndef STDIO_REDIRECT_H
#define STDIO_REDIRECT_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Input masking modes
 */
typedef enum {
    INPUT_MODE_NORMAL,        // Display characters as-is
    INPUT_MODE_MASKED,        // Display asterisks (*) for all input
    INPUT_MODE_REVEAL_LAST    // Display last character briefly, then mask
} input_mode_t;

/**
 * @brief Initialize STDIO redirection
 * 
 * Must be called after lcd_init() and keypad_init().
 * 
 * @return ESP_OK on success
 */
esp_err_t stdio_redirect_init(void);

/**
 * @brief Printf-like function that outputs to LCD
 * 
 * Formats and displays text on the LCD using printf-style formatting.
 * 
 * @param format Printf-style format string
 * @param ... Variable arguments
 * @return Number of characters written
 */
int lcd_printf(const char *format, ...);

/**
 * @brief Set input mode for lcd_scanf
 * 
 * @param mode Input mode (NORMAL, MASKED, or REVEAL_LAST)
 */
void lcd_scanf_set_mode(input_mode_t mode);

/**
 * @brief Enable/disable centered input display
 * 
 * @param centered true to center input, false for normal
 */
void lcd_scanf_set_centered(bool centered);

/**
 * @brief Set character filter for input (e.g., digits only)
 * 
 * @param digits_only true to accept only 0-9, false to accept all
 */
void lcd_scanf_set_digits_only(bool digits_only);

/**
 * @brief Scanf-like function that reads from keypad
 * 
 * Reads input from keypad until '#' is pressed. Supports:
 * - Normal keys: Echo to LCD, add to buffer
 * - '*' (backspace): Remove last character
 * - '#' (enter): End input
 * 
 * @param format Scanf-style format string (typically "%s" for strings)
 * @param ... Variable arguments (pointers to store results)
 * @return Number of successfully parsed items
 */
int lcd_scanf(const char *format, ...);

#endif // STDIO_REDIRECT_H
