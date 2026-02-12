#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Initialize LCD2004 with i2c communication via PCF8574
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
 * @param col Column position (0-19)
 * @param row Row position (0-3)
 */
void lcd_set_cursor(uint8_t col, uint8_t row);

/**
 * @brief Write a single character to LCD
 * 
 * Handles special characters:
 * - '\n' moves to next line
 * - '\r' returns to start of current line
 * - Automatically wraps at end of line
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
 * @brief Move cursor to home position (0,0)
 */
void lcd_home(void);

/**
 * @brief Clear a specific row on the LCD
 * 
 * @param row Row to clear (0-3)
 */
void lcd_clear_row(uint8_t row);

#endif // LCD_I2C_H
