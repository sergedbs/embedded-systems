/**
 * @file lock_ui.c
 * @brief UI/display helper functions for lock system
 */

#include "lock_ui.h"
#include "lcd_i2c.h"
#include "led.h"
#include "buzzer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define LCD_COLS 20
#define LCD_ROWS 4

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
    
    // Clear the row with spaces
    lcd_set_cursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    
    // Display centered text
    lcd_set_cursor(start_col, row);
    for (size_t i = 0; i < text_len; i++) {
        lcd_putc(text[i]);
    }
}

void lock_ui_display_title(const char *title)
{
    lcd_clear();
    
    // Top border
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc('=');
    }
    lcd_putc('\n');
    
    // Title centered
    lock_ui_display_centered(title, 1);
    
    // Bottom border
    lcd_set_cursor(0, 2);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc('=');
    }
}

void lock_ui_display_error(const char *message, uint32_t delay_ms)
{
    lcd_clear();
    lock_ui_display_centered("ERROR!", 1);
    lock_ui_display_centered(message, 2);
    
    led_error();
    buzzer_error();
    
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    led_all_off();
}

void lock_ui_display_success(const char *message, uint32_t delay_ms)
{
    lcd_clear();
    lock_ui_display_centered("SUCCESS!", 1);
    lock_ui_display_centered(message, 2);
    
    led_success();
    buzzer_success();
    
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    led_all_off();
}
