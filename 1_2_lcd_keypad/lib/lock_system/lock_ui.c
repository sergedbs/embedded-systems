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
    
    // Clear only the area needed (optimization: no full-row clear for static text)
    lcd_set_cursor(start_col, row);
    
    // Display centered text
    for (size_t i = 0; i < text_len; i++) {
        lcd_putc(text[i]);
    }
}

void lock_ui_display_title(const char *title)
{
    // Clear rows 0-2
    lcd_set_cursor(0, 0);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    
    // Title centered on row 1
    lcd_set_cursor(0, 1);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    lock_ui_display_centered(title, 1);
    
    // Clear row 2
    lcd_set_cursor(0, 2);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
}

void lock_ui_display_error(const char *message, uint32_t delay_ms)
{
    // Clear rows 1-3 for error message (includes input row)
    lcd_set_cursor(0, 1);
    for (uint8_t i = 0; i < LCD_COLS * 3; i++) {
        lcd_putc(' ');
    }
    
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
    // Clear rows 1-3 for success message (includes input row)
    lcd_set_cursor(0, 1);
    for (uint8_t i = 0; i < LCD_COLS * 3; i++) {
        lcd_putc(' ');
    }
    
    lock_ui_display_centered("SUCCESS!", 1);
    lock_ui_display_centered(message, 2);
    
    led_success();
    buzzer_success();
    
    if (delay_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    led_all_off();
}

void lock_ui_clear_content(void)
{
    // Clear only rows 2-3 (content area), keeping title intact
    lcd_set_cursor(0, 2);
    for (uint8_t i = 0; i < LCD_COLS * 2; i++) {
        lcd_putc(' ');
    }
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
}
