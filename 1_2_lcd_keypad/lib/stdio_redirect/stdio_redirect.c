/**
 * @file stdio_redirect.c
 * @brief STDIO retargeting implementation for ESP32
 * 
 * Provides wrapper functions to redirect I/O to LCD and keypad.
 */

#include "stdio_redirect.h"
#include "config_pins.h"
#include "config_app.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lcd_i2c.h"
#include "keypad.h"
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

// Input mode settings
static input_mode_t current_input_mode = INPUT_MODE_NORMAL;
static bool digits_only_filter = false;

// Keypress callback — set by higher layer to avoid upward dependency
static void (*s_keypress_cb)(void) = NULL;

// Character reveal state (non-blocking timer)
static struct {
    TimerHandle_t timer;
    char         *buffer;
    size_t        length;
    uint8_t       row;
    uint8_t       col;
} s_reveal = {0};

/**
 * @brief Timer callback to mask the last character after reveal delay
 */
static void reveal_timer_callback(TimerHandle_t xTimer)
{
    if (s_reveal.buffer && s_reveal.length > 0) {
        lcd_set_cursor(s_reveal.col + s_reveal.length - 1, s_reveal.row);
        lcd_putc('*');
    }
}

/**
 * @brief Initialize STDIO redirection
 */
esp_err_t stdio_redirect_init(void)
{
    s_reveal.timer = xTimerCreate(
        "reveal_timer",
        pdMS_TO_TICKS(CHAR_REVEAL_MS),
        pdFALSE,
        NULL,
        reveal_timer_callback
    );

    if (s_reveal.timer == NULL) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief Register a callback invoked on every keypress
 */
void lcd_scanf_set_keypress_cb(void (*cb)(void))
{
    s_keypress_cb = cb;
}

/**
 * @brief Configure input mode and digit filter
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
    
    // Clear remaining characters to end of line
    size_t chars_to_clear = LCD_COLS - start_col - length;
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

    uint8_t start_col = lcd_get_cursor_col();
    uint8_t start_row = lcd_get_cursor_row();

    // Clear the input area (preserve prompt before start_col)
    for (uint8_t i = start_col; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    lcd_set_cursor(start_col, start_row);

    while (idx < sizeof(buffer) - 1) {
        char key = keypad_getkey_blocking();

        if (s_keypress_cb) s_keypress_cb();

        if (key == 'C') {
            if (s_reveal.timer != NULL && xTimerIsTimerActive(s_reveal.timer)) {
                xTimerStop(s_reveal.timer, 0);
            }
            current_input_mode = INPUT_MODE_NORMAL;
            digits_only_filter = false;
            return -1;
        }

        if (digits_only_filter) {
            if (key != '*' && key != '#' && (key < '0' || key > '9')) {
                buzzer_beep(30);
                continue;
            }
        }

        if (key == '#') {
            buffer[idx] = '\0';
            if (s_reveal.timer != NULL && xTimerIsTimerActive(s_reveal.timer)) {
                xTimerStop(s_reveal.timer, 0);
            }
            if (current_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                lcd_set_cursor(start_col + idx - 1, start_row);
                lcd_putc('*');
            }
            break;
        } else if (key == '*') {
            if (idx > 0) {
                if (s_reveal.timer != NULL && xTimerIsTimerActive(s_reveal.timer)) {
                    xTimerStop(s_reveal.timer, 0);
                }
                idx--;
                display_input(buffer, idx, start_row, start_col);
            }
        } else {
            buffer[idx++] = key;
            display_input(buffer, idx, start_row, start_col);

            if (current_input_mode == INPUT_MODE_REVEAL_LAST && idx > 0) {
                if (s_reveal.timer != NULL && xTimerIsTimerActive(s_reveal.timer)) {
                    xTimerStop(s_reveal.timer, 0);
                }
                s_reveal.buffer = buffer;
                s_reveal.length = idx;
                s_reveal.row    = start_row;
                s_reveal.col    = start_col;
                xTimerStart(s_reveal.timer, 0);
            }
        }
    }

    buffer[idx] = '\0';

    va_list args;
    va_start(args, format);
    int ret = vsscanf(buffer, format, args);
    va_end(args);

    current_input_mode = INPUT_MODE_NORMAL;
    digits_only_filter = false;
    s_reveal.buffer = NULL;
    s_reveal.length = 0;

    return ret;
}
