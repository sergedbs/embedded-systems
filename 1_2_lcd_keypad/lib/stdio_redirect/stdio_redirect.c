/**
 * @file stdio_redirect.c
 * @brief STDIO retargeting implementation for ESP32
 * 
 * Provides wrapper functions to redirect I/O to LCD and keypad.
 */

#include "stdio_redirect.h"
#include "config_pins.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lcd_i2c.h"
#include "keypad.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_COLS 20

// Input mode settings
static input_mode_t current_input_mode = INPUT_MODE_NORMAL;
static bool centered_input = false;

/**
 * @brief Initialize STDIO redirection
 */
esp_err_t stdio_redirect_init(void)
{
    // Nothing special needed for wrapper-based approach
    return ESP_OK;
}

/**
 * @brief Set input mode for lcd_scanf
 */
void lcd_scanf_set_mode(input_mode_t mode)
{
    current_input_mode = mode;
}

/**
 * @brief Enable/disable centered input display
 */
void lcd_scanf_set_centered(bool centered)
{
    centered_input = centered;
}

/**
 * @brief Printf-like function that outputs to LCD
 */
int lcd_printf(const char *format, ...)
{
    char buffer[128];
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    lcd_print(buffer);
    return ret;
}

/**
 * @brief Helper function to display input with masking and centering
 */
static void display_input(const char *buffer, size_t length, uint8_t row, uint8_t start_col)
{
    // Clear the display area
    lcd_set_cursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    
    // Calculate centered position if needed
    uint8_t col = start_col;
    if (centered_input && length > 0) {
        col = (LCD_COLS - length) / 2;
    }
    
    // Display the input based on mode
    lcd_set_cursor(col, row);
    
    if (current_input_mode == INPUT_MODE_NORMAL) {
        // Display as-is
        for (size_t i = 0; i < length; i++) {
            lcd_putc(buffer[i]);
        }
    } else if (current_input_mode == INPUT_MODE_MASKED) {
        // Display all as asterisks
        for (size_t i = 0; i < length; i++) {
            lcd_putc('*');
        }
    } else if (current_input_mode == INPUT_MODE_REVEAL_LAST) {
        // Display all but last as asterisks, last as actual char
        for (size_t i = 0; i < length; i++) {
            if (i == length - 1) {
                lcd_putc(buffer[i]);  // Show last character
            } else {
                lcd_putc('*');  // Mask previous characters
            }
        }
    }
}

/**
 * @brief Scanf-like function that reads from keypad
 * 
 * Supports masking, last-char reveal, and centered input.
 */
int lcd_scanf(const char *format, ...)
{
    char buffer[64];
    size_t idx = 0;
    
    // Get current cursor position
    uint8_t start_row = 3;  // Assume we're on row 3 for input
    uint8_t start_col = 0;
    
    // Read characters until enter ('#') is pressed
    while (idx < sizeof(buffer) - 1) {
        char key = keypad_getkey_blocking();
        
        if (key == '#') {
            // Enter pressed - end input
            buffer[idx] = '\0';
            
            // If in REVEAL_LAST mode, mask the last character before exit
            if (current_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                display_input(buffer, idx, start_row, start_col);
            }
            
            break;
        } else if (key == '*') {
            // Backspace - remove last character
            if (idx > 0) {
                idx--;
                display_input(buffer, idx, start_row, start_col);
            }
        } else {
            // Normal character - add to buffer
            buffer[idx++] = key;
            
            // Display immediately
            display_input(buffer, idx, start_row, start_col);
            
            // If in REVEAL_LAST mode, wait 500ms then mask the character
            if (current_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                vTaskDelay(pdMS_TO_TICKS(CHAR_REVEAL_MS));
                display_input(buffer, idx, start_row, start_col);
            }
        }
    }
    
    buffer[idx] = '\0';
    
    // Parse the input using sscanf with the provided format
    va_list args;
    va_start(args, format);
    int ret = vsscanf(buffer, format, args);
    va_end(args);
    
    // Reset modes to defaults
    current_input_mode = INPUT_MODE_NORMAL;
    centered_input = false;
    
    return ret;
}
