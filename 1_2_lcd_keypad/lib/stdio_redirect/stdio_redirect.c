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
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#define LCD_COLS 20

// Input mode settings
static input_mode_t current_input_mode = INPUT_MODE_NORMAL;
static bool digits_only_filter = false;

// Character reveal timer (non-blocking)
static TimerHandle_t reveal_timer = NULL;
static char *reveal_buffer = NULL;
static size_t reveal_length = 0;
static uint8_t reveal_row = 0;
static uint8_t reveal_col = 0;

/**
 * @brief Timer callback to mask the last character after reveal delay
 */
static void reveal_timer_callback(TimerHandle_t xTimer)
{
    if (reveal_buffer && reveal_length > 0) {
        // Mask the last character
        lcd_set_cursor(reveal_col + reveal_length - 1, reveal_row);
        lcd_putc('*');
    }
}

/**
 * @brief Initialize STDIO redirection
 */
esp_err_t stdio_redirect_init(void)
{
    // Create the reveal timer (one-shot, non-auto-reload)
    reveal_timer = xTimerCreate(
        "reveal_timer",
        pdMS_TO_TICKS(CHAR_REVEAL_MS),
        pdFALSE,  // One-shot timer
        NULL,
        reveal_timer_callback
    );
    
    if (reveal_timer == NULL) {
        return ESP_FAIL;
    }
    
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
 * @brief Set digits-only filter for PIN entry
 */
void lcd_scanf_set_digits_only(bool digits_only)
{
    digits_only_filter = digits_only;
}

/**
 * @brief Unified configuration for lcd_scanf (reduces redundancy)
 */
void lcd_scanf_configure(input_mode_t mode, bool digits_only)
{
    current_input_mode = mode;
    digits_only_filter = digits_only;
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
 * @brief Helper function to display input with masking (left-aligned, no flicker)
 */
static void display_input(const char *buffer, size_t length, uint8_t row, uint8_t start_col)
{
    // Set cursor to start position
    lcd_set_cursor(start_col, row);
    
    // Display the input based on mode
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
    
    // Clear remaining characters to end of line (prevents leftover text)
    size_t chars_to_clear = (start_col + PIN_MAX_LENGTH) - (start_col + length);
    for (size_t i = 0; i < chars_to_clear; i++) {
        lcd_putc(' ');
    }
}

/**
 * @brief Scanf-like function that reads from keypad
 * 
 * Supports masking, non-blocking last-char reveal, and left-aligned input.
 */
int lcd_scanf(const char *format, ...)
{
    char buffer[64];
    size_t idx = 0;
    
    // Get current cursor position (right after the prompt)
    uint8_t start_col = lcd_get_cursor_col();
    uint8_t start_row = lcd_get_cursor_row();
    
    // Clear only the input area (preserve prompts before start_col)
    for (uint8_t i = start_col; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    lcd_set_cursor(start_col, start_row);
    
    // Read characters until enter ('#') is pressed
    while (idx < sizeof(buffer) - 1) {
        char key = keypad_getkey_blocking();
        
        // Filter out invalid characters if digits-only mode is enabled
        if (digits_only_filter) {
            // Only allow 0-9, *, and # (backspace and enter)
            if (key != '*' && key != '#' && (key < '0' || key > '9')) {
                // Invalid character - short error beep
                buzzer_beep(30);
                continue;  // Skip this character
            }
        }
        
        if (key == '#') {
            // Enter pressed - end input
            buffer[idx] = '\0';
            
            // Stop reveal timer if active
            if (reveal_timer != NULL && xTimerIsTimerActive(reveal_timer)) {
                xTimerStop(reveal_timer, 0);
            }
            
            // If in REVEAL_LAST mode, mask the last character immediately
            if (current_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                lcd_set_cursor(start_col + idx - 1, start_row);
                lcd_putc('*');
            }
            
            break;
        } else if (key == '*') {
            // Backspace - remove last character
            if (idx > 0) {
                // Stop reveal timer if active
                if (reveal_timer != NULL && xTimerIsTimerActive(reveal_timer)) {
                    xTimerStop(reveal_timer, 0);
                }
                
                idx--;
                display_input(buffer, idx, start_row, start_col);
            }
        } else {
            // Normal character - add to buffer
            buffer[idx++] = key;
            
            // Display immediately
            display_input(buffer, idx, start_row, start_col);
            
            // If in REVEAL_LAST mode, start non-blocking timer to mask character
            if (current_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                // Stop any existing timer
                if (reveal_timer != NULL && xTimerIsTimerActive(reveal_timer)) {
                    xTimerStop(reveal_timer, 0);
                }
                
                // Update timer context
                reveal_buffer = buffer;
                reveal_length = idx;
                reveal_row = start_row;
                reveal_col = start_col;
                
                // Start timer (non-blocking - input loop continues immediately)
                xTimerStart(reveal_timer, 0);
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
    digits_only_filter = false;
    
    // Clear timer context
    reveal_buffer = NULL;
    reveal_length = 0;
    
    return ret;
}
