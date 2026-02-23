#include "lcd_i2c.h"
#include "config_pins.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "LCD_I2C";

// PCF8574 Pin mapping (bit positions)
#define LCD_RS      (1 << 0)    // P0 - Register Select
#define LCD_RW      (1 << 1)    // P1 - Read/Write (always 0 for write)
#define LCD_EN      (1 << 2)    // P2 - Enable
#define LCD_BL      (1 << 3)    // P3 - Backlight
#define LCD_D4      (1 << 4)    // P4 - Data bit 4
#define LCD_D5      (1 << 5)    // P5 - Data bit 5
#define LCD_D6      (1 << 6)    // P6 - Data bit 6
#define LCD_D7      (1 << 7)    // P7 - Data bit 7

// HD44780 Commands
#define LCD_CMD_CLEAR           0x01
#define LCD_CMD_HOME            0x02
#define LCD_CMD_ENTRY_MODE      0x06    // Increment cursor, no display shift
#define LCD_CMD_DISPLAY_ON      0x0C    // Display on, cursor off, blink off
#define LCD_CMD_DISPLAY_OFF     0x08
#define LCD_CMD_CURSOR_SHIFT    0x10
#define LCD_CMD_FUNCTION_SET    0x28    // 4-bit, 2 lines, 5x8 font
#define LCD_CMD_SET_CGRAM       0x40
#define LCD_CMD_SET_DDRAM       0x80

// LCD state
static uint8_t lcd_i2c_addr = LCD_I2C_ADDR_1;
static uint8_t backlight_state = LCD_BL;    // Backlight on by default
static uint8_t current_col = 0;
static uint8_t current_row = 0;

// Row start addresses for 20x4 LCD
static const uint8_t row_offsets[4] = {0x00, 0x40, 0x14, 0x54};

/**
 * @brief Write a byte to PCF8574 via i2c
 */
static esp_err_t pcf8574_write(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (lcd_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(LCD_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

/**
 * @brief Pulse the enable pin to latch data
 */
static void lcd_pulse_enable(uint8_t data)
{
    pcf8574_write(data | LCD_EN);
    vTaskDelay(pdMS_TO_TICKS(1));  // EN pulse must be >450ns
    pcf8574_write(data & ~LCD_EN);
    vTaskDelay(pdMS_TO_TICKS(1));  // Command needs >37us to settle
}

/**
 * @brief Send 4 bits to LCD
 */
static void lcd_write_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = backlight_state | mode;  // Keep backlight state
    data |= (nibble & 0x0F) << 4;           // Map nibble to D4-D7
    
    pcf8574_write(data);
    lcd_pulse_enable(data);
}

/**
 * @brief Send a byte to LCD in 4-bit mode (high nibble first, then low nibble)
 */
static void lcd_send(uint8_t value, uint8_t mode)
{
    lcd_write_nibble(value >> 4, mode);     // High nibble
    lcd_write_nibble(value & 0x0F, mode);   // Low nibble
}

/**
 * @brief Send command to LCD
 */
static void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, 0);           // RS=0 for command
    vTaskDelay(pdMS_TO_TICKS(2));
}

/**
 * @brief Send data to LCD
 */
static void lcd_write_data(uint8_t data)
{
    lcd_send(data, LCD_RS);     // RS=1 for data
    vTaskDelay(pdMS_TO_TICKS(1));
}

esp_err_t lcd_init(void)
{
    esp_err_t ret;
    
    // Configure i2c master
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_LCD_SDA,
        .scl_io_num = GPIO_LCD_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = LCD_I2C_FREQ_HZ,
    };
    
    ret = i2c_param_config(LCD_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = i2c_driver_install(LCD_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "i2c initialized");
    
    // Wait for LCD to power up
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Try to detect LCD at default address
    ret = pcf8574_write(backlight_state);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LCD not found at 0x%02X, trying 0x%02X", lcd_i2c_addr, LCD_I2C_ADDR_2);
        lcd_i2c_addr = LCD_I2C_ADDR_2;
        ret = pcf8574_write(backlight_state);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "LCD not found at either address");
            return ret;
        }
    }
    
    ESP_LOGI(TAG, "LCD found at address 0x%02X", lcd_i2c_addr);
    
    // HD44780 initialization sequence for 4-bit mode
    // Reference: https://www.sparkfun.com/datasheets/LCD/HD44780.pdf (page 46)
    
    // Initial sequence: send 0x03 three times
    lcd_write_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    lcd_write_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    lcd_write_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Switch to 4-bit mode
    lcd_write_nibble(0x02, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Function set: 4-bit, 2 lines, 5x8 font
    lcd_command(LCD_CMD_FUNCTION_SET);
    
    // Display off
    lcd_command(LCD_CMD_DISPLAY_OFF);
    
    // Clear display
    lcd_command(LCD_CMD_CLEAR);
    vTaskDelay(pdMS_TO_TICKS(2));
    
    // Entry mode: increment cursor, no shift
    lcd_command(LCD_CMD_ENTRY_MODE);
    
    // Display on, cursor off, blink off
    lcd_command(LCD_CMD_DISPLAY_ON);
    
    current_col = 0;
    current_row = 0;
    
    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}

void lcd_clear(void)
{
    lcd_command(LCD_CMD_CLEAR);
    vTaskDelay(pdMS_TO_TICKS(2));
    current_col = 0;
    current_row = 0;
}

void lcd_home(void)
{
    lcd_command(LCD_CMD_HOME);
    vTaskDelay(pdMS_TO_TICKS(2));
    current_col = 0;
    current_row = 0;
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    if (row >= LCD_ROWS) row = LCD_ROWS - 1;
    if (col >= LCD_COLS) col = LCD_COLS - 1;
    
    current_col = col;
    current_row = row;
    
    uint8_t address = row_offsets[row] + col;
    lcd_command(LCD_CMD_SET_DDRAM | address);
}

void lcd_putc(char c)
{
    if (c == '\n') {
        // Newline: move to start of next row
        current_row++;
        if (current_row >= LCD_ROWS) {
            current_row = 0;  // Wrap to top
        }
        current_col = 0;
        lcd_set_cursor(current_col, current_row);
        return;
    }
    
    if (c == '\r') {
        // Carriage return: move to start of current row
        current_col = 0;
        lcd_set_cursor(current_col, current_row);
        return;
    }
    
    if (c == '\b') {
        // Backspace: move cursor back one position
        if (current_col > 0) {
            current_col--;
            lcd_set_cursor(current_col, current_row);
        } else if (current_row > 0) {
            // If at start of line, move to end of previous line
            current_row--;
            current_col = LCD_COLS - 1;
            lcd_set_cursor(current_col, current_row);
        }
        return;
    }
    
    // Write character
    lcd_write_data((uint8_t)c);
    current_col++;
    
    // Handle line wrapping
    if (current_col >= LCD_COLS) {
        current_col = 0;
        current_row++;
        if (current_row >= LCD_ROWS) {
            current_row = 0;  // Wrap to top
        }
        lcd_set_cursor(current_col, current_row);
    }
}

void lcd_print(const char *str)
{
    if (str == NULL) return;
    
    while (*str) {
        lcd_putc(*str++);
    }
}

void lcd_backlight(bool on)
{
    backlight_state = on ? LCD_BL : 0;
    pcf8574_write(backlight_state);
}

void lcd_clear_row(uint8_t row)
{
    if (row >= LCD_ROWS) return;
    
    lcd_set_cursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; i++) {
        lcd_putc(' ');
    }
    lcd_set_cursor(0, row);
}

uint8_t lcd_get_cursor_col(void)
{
    return current_col;
}

uint8_t lcd_get_cursor_row(void)
{
    return current_row;
}
