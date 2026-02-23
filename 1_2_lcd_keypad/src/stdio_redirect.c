/**
 * @file stdio_redirect.c
 * @brief STDIO retargeting implementation for ESP32
 * 
 * Provides wrapper functions to redirect I/O to LCD and keypad.
 */

#include "stdio_redirect.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "lcd_i2c.h"
#include "keypad.h"

/**
 * @brief Initialize STDIO redirection
 */
esp_err_t stdio_redirect_init(void)
{
    // Nothing special needed for wrapper-based approach
    return ESP_OK;
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
 * @brief Scanf-like function that reads from keypad
 * 
 * Simplified version that reads a string until '#' is pressed.
 */
int lcd_scanf(const char *format, ...)
{
    char buffer[64];
    size_t idx = 0;
    
    // Read characters until enter ('#') is pressed
    while (idx < sizeof(buffer) - 1) {
        char key = keypad_getkey_blocking();
        
        if (key == '#') {
            // Enter pressed - end input
            buffer[idx] = '\0';
            lcd_putc('\n');  // Move to next line
            break;
        } else if (key == '*') {
            // Backspace - remove last character
            if (idx > 0) {
                idx--;
                lcd_putc('\b');
                lcd_putc(' ');
                lcd_putc('\b');
            }
        } else {
            // Normal character
            buffer[idx++] = key;
            lcd_putc(key);  // Echo to LCD
        }
    }
    
    buffer[idx] = '\0';
    
    // Parse the input using sscanf with the provided format
    va_list args;
    va_start(args, format);
    int ret = vsscanf(buffer, format, args);
    va_end(args);
    
    return ret;
}