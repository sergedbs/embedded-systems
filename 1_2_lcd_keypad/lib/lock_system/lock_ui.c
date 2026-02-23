/**
 * @file lock_ui.c
 * @brief UI/display helper functions for lock system (16x2 LCD)
 */

#include "lock_ui.h"
#include "lcd_i2c.h"
#include "led.h"
#include "buzzer.h"
#include "config_pins.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

void lock_ui_display_centered(const char *text, uint8_t row)
{
    if (row >= LCD_ROWS) {
        return;
    }
    
    size_t text_len = strlen(text);
    if (text_len > LCD_COLS) {
        text_len = LCD_COLS;
    }
    
    // Calculate starting column for centered text
    uint8_t start_col = (LCD_COLS - text_len) / 2;
    
    // Build complete row buffer (16 chars exactly)
    char row_buffer[LCD_COLS + 1];
    memset(row_buffer, ' ', LCD_COLS);
    row_buffer[LCD_COLS] = '\0';
    
    // Copy text to center position
    memcpy(row_buffer + start_col, text, text_len);
    
    // Write entire row - set cursor and use lcd_print
    lcd_set_cursor(0, row);
    
    // Write only 15 characters to avoid cursor wrap issue
    for (uint8_t i = 0; i < LCD_COLS - 1; i++) {
        lcd_putc(row_buffer[i]);
    }
    
    // Manually set cursor to last position and write last char
    lcd_set_cursor(LCD_COLS - 1, row);
    lcd_putc(row_buffer[LCD_COLS - 1]);
}

void lock_ui_display_title(const char *title)
{
    // For 16x2: Display title on row 0 (centered function will clear it)
    lock_ui_display_centered(title, 0);
}

void lock_ui_display_error(const char *message, uint32_t delay_ms)
{
    // For 16x2: Display error on row 1
    lock_ui_clear_row(1);
    lock_ui_display_centered(message, 1);
    
    led_error();
    buzzer_error();
    
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    led_all_off();
}

void lock_ui_display_success(const char *message, uint32_t delay_ms)
{
    // For 16x2: Display success on row 1
    lock_ui_clear_row(1);
    lock_ui_display_centered(message, 1);
    
    led_success();
    buzzer_success();
    
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    led_all_off();
}

void lock_ui_clear_content(void)
{
    // For 16x2: Clear row 1 (content/input area)
    lock_ui_clear_row(1);
}

void lock_ui_clear_row(uint8_t row)
{
    if (row >= LCD_ROWS) {
        return;
    }
    
    lcd_set_cursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    
    // Reset cursor to start of row for consistency
    lcd_set_cursor(0, row);
}
