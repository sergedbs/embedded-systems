#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_putc(char c);
void lcd_print(const char *str);
void lcd_backlight(bool on);
void lcd_clear_row(uint8_t row);

#endif // LCD_I2C_H
