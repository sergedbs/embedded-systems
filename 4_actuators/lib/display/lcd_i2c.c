#include "lcd_i2c.h"

#include <string.h>

#include "app_config.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lcd_i2c";

#define LCD_RS                 (1 << 0)
#define LCD_EN                 (1 << 2)
#define LCD_BL                 (1 << 3)
#define LCD_CMD_CLEAR          0x01
#define LCD_CMD_HOME           0x02
#define LCD_CMD_ENTRY_MODE     0x06
#define LCD_CMD_DISPLAY_ON     0x0C
#define LCD_CMD_DISPLAY_OFF    0x08
#define LCD_CMD_FUNCTION_SET   0x28
#define LCD_CMD_SET_DDRAM      0x80

static uint8_t s_addr = LCD_I2C_ADDR_1;
static uint8_t s_backlight = LCD_BL;
static uint8_t s_col;
static uint8_t s_row;
static const uint8_t s_row_offsets[LCD_ROWS] = {0x00, 0x40};

static esp_err_t pcf8574_write(uint8_t data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (s_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    const esp_err_t ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static void lcd_pulse_enable(uint8_t data)
{
    (void)pcf8574_write(data | LCD_EN);
    (void)pcf8574_write(data & (uint8_t)~LCD_EN);
}

static void lcd_write_nibble(uint8_t nibble, uint8_t mode)
{
    uint8_t data = s_backlight | mode;
    data |= (uint8_t)((nibble & 0x0F) << 4);
    (void)pcf8574_write(data);
    lcd_pulse_enable(data);
}

static void lcd_send(uint8_t value, uint8_t mode)
{
    lcd_write_nibble(value >> 4, mode);
    lcd_write_nibble(value & 0x0F, mode);
}

static void lcd_command(uint8_t cmd)
{
    lcd_send(cmd, 0);
    if (cmd == LCD_CMD_CLEAR || cmd == LCD_CMD_HOME) {
        esp_rom_delay_us(2000);
    }
}

static void lcd_write_data(uint8_t data)
{
    lcd_send(data, LCD_RS);
}

esp_err_t lcd_init(void)
{
    const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t ret = i2c_param_config(I2C_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    ret = pcf8574_write(s_backlight);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LCD not found at 0x%02X, trying 0x%02X", s_addr, LCD_I2C_ADDR_2);
        s_addr = LCD_I2C_ADDR_2;
        ret = pcf8574_write(s_backlight);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "LCD not found");
            return ret;
        }
    }

    lcd_write_nibble(0x03, 0);
    esp_rom_delay_us(5000);
    lcd_write_nibble(0x03, 0);
    esp_rom_delay_us(200);
    lcd_write_nibble(0x03, 0);
    esp_rom_delay_us(200);
    lcd_write_nibble(0x02, 0);
    esp_rom_delay_us(200);

    lcd_command(LCD_CMD_FUNCTION_SET);
    lcd_command(LCD_CMD_DISPLAY_OFF);
    lcd_command(LCD_CMD_CLEAR);
    lcd_command(LCD_CMD_ENTRY_MODE);
    lcd_command(LCD_CMD_DISPLAY_ON);

    s_col = 0;
    s_row = 0;
    ESP_LOGI(TAG, "LCD initialized at 0x%02X", s_addr);
    return ESP_OK;
}

void lcd_clear(void)
{
    lcd_command(LCD_CMD_CLEAR);
    s_col = 0;
    s_row = 0;
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    if (row >= LCD_ROWS) {
        row = LCD_ROWS - 1;
    }
    if (col >= LCD_COLS) {
        col = LCD_COLS - 1;
    }

    s_col = col;
    s_row = row;
    lcd_command(LCD_CMD_SET_DDRAM | (uint8_t)(s_row_offsets[row] + col));
}

void lcd_putc(char c)
{
    if (c == '\n') {
        s_row = (uint8_t)((s_row + 1U) % LCD_ROWS);
        s_col = 0;
        lcd_set_cursor(s_col, s_row);
        return;
    }
    if (c == '\r') {
        s_col = 0;
        lcd_set_cursor(s_col, s_row);
        return;
    }

    lcd_write_data((uint8_t)c);
    if (s_col < LCD_COLS - 1) {
        s_col++;
    }
}

void lcd_print(const char *str)
{
    if (str == NULL) {
        return;
    }

    while (*str != '\0') {
        lcd_putc(*str++);
    }
}

void lcd_backlight(bool on)
{
    s_backlight = on ? LCD_BL : 0;
    (void)pcf8574_write(s_backlight);
}

void lcd_clear_row(uint8_t row)
{
    if (row >= LCD_ROWS) {
        return;
    }

    lcd_set_cursor(0, row);
    for (uint8_t i = 0; i < LCD_COLS; ++i) {
        lcd_putc(' ');
    }
    lcd_set_cursor(0, row);
}
