#include "lcd_display.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "lcd_i2c.h"

static bool s_ready;
static char s_last_line1[LCD_COLS + 1];
static char s_last_line2[LCD_COLS + 1];

static const char *mode_text(control_mode_t mode)
{
    return mode == CONTROL_MODE_PID ? "PID" : "ONF";
}

static const char *status_text(system_status_t status)
{
    switch (status) {
        case SYSTEM_STATUS_WARN:
            return "WRN";
        case SYSTEM_STATUS_ALERT:
            return "ALR";
        case SYSTEM_STATUS_OK:
        default:
            return "OK ";
    }
}

esp_err_t lcd_display_init(void)
{
    const esp_err_t err = lcd_init();
    s_ready = (err == ESP_OK);
    if (s_ready) {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Lab5 Auto Ctrl");
        lcd_set_cursor(0, 1);
        lcd_print("Starting...");
        memset(s_last_line1, 0, sizeof(s_last_line1));
        memset(s_last_line2, 0, sizeof(s_last_line2));
    }
    return err;
}

static void format_lcd_line(char *out, size_t out_size, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(out, out_size, fmt, args);
    va_end(args);

    const size_t len = strlen(out);
    for (size_t i = len; i < LCD_COLS && i < out_size - 1U; ++i) {
        out[i] = ' ';
    }
    out[LCD_COLS] = '\0';
}

static void write_line_if_changed(uint8_t row, const char *line, char *last)
{
    if (strncmp(line, last, LCD_COLS) == 0) {
        return;
    }

    lcd_set_cursor(0, row);
    lcd_print(line);
    memcpy(last, line, LCD_COLS + 1U);
}

esp_err_t lcd_display_render(const system_state_t *snapshot)
{
    if (!s_ready || snapshot == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    char line1[LCD_COLS + 1];
    char line2[LCD_COLS + 1];

    if (!snapshot->sensor_valid) {
        format_lcd_line(line1, sizeof(line1), "%s SP:%4.1fC",
                        mode_text(snapshot->mode),
                        snapshot->setpoint_c);
        format_lcd_line(line2, sizeof(line2), "DHT wait %s",
                        status_text(snapshot->status));
    } else {
        format_lcd_line(line1, sizeof(line1), "%s T:%4.1f/%4.1f",
                        mode_text(snapshot->mode),
                        snapshot->filtered_temperature_c,
                        snapshot->setpoint_c);
        format_lcd_line(line2, sizeof(line2), "Out:%3.0f%% R:%u %s",
                        snapshot->output_pct,
                        snapshot->relay_output ? 1U : 0U,
                        status_text(snapshot->status));
    }

    write_line_if_changed(0, line1, s_last_line1);
    write_line_if_changed(1, line2, s_last_line2);
    return ESP_OK;
}

bool lcd_display_is_ready(void)
{
    return s_ready;
}
