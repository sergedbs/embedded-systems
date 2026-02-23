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
