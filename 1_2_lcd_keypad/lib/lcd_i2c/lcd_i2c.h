#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize LCD1602 (16x2) with i2c communication via PCF8574
 * 
 * Initializes i2c bus and LCD in 4-bit mode with proper HD44780 sequence.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lcd_init(void);

/**
 * @brief Clear the LCD display and move cursor to home position
 */
void lcd_clear(void);

/**
 * @brief Set cursor position on LCD
 * 
 * @param col Column position (0-15)
 * @param row Row position (0-1)
 */
void lcd_set_cursor(uint8_t col, uint8_t row);

/**
 * @brief Write a single character to the current cursor position
 *
 * Handles '\n' (next row), '\r' (start of current row), '\b' (back one col).
 * Writing at column 15 stops at column 15 — no auto-wrap (HD44780 16x2 DDRAM
 * rows are non-contiguous: row 0 ends at 0x0F, row 1 starts at 0x40).
 *
 * @param c Character to write
 */
void lcd_putc(char c);

/**
 * @brief Print a string to LCD
 * 
 * @param str Null-terminated string to print
 */
void lcd_print(const char *str);

/**
 * @brief Control LCD backlight
 * 
 * @param on true to turn backlight on, false to turn off
 */
void lcd_backlight(bool on);

/**
 * @brief Clear a specific row on the LCD
 * 
 * @param row Row to clear (0-1)
 */
void lcd_clear_row(uint8_t row);

/**
 * @brief Get current cursor column position
 * 
 * @return Current column (0-15)
 */
uint8_t lcd_get_cursor_col(void);

/**
 * @brief Get current cursor row position
 * 
 * @return Current row (0-1)
 */
uint8_t lcd_get_cursor_row(void);

#endif // LCD_I2C_H
