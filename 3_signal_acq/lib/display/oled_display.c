#include "oled_display.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define OLED_WIDTH            128
#define OLED_HEIGHT           64
#define OLED_PAGES            (OLED_HEIGHT / 8)
#define OLED_FB_SIZE          (OLED_WIDTH * OLED_PAGES)
#define OLED_I2C_TIMEOUT_MS   100

static const char *TAG = "oled_display";
static uint8_t s_fb[OLED_FB_SIZE];
static bool s_ready = false;

static uint8_t light_percent_from_raw(uint16_t raw)
{
    if (LIGHT_RAW_MAX <= LIGHT_RAW_MIN) {
        return 0;
    }

    if (raw > LIGHT_RAW_MAX) {
        raw = LIGHT_RAW_MAX;
    }
#if LIGHT_RAW_MIN > 0
    else if (raw < LIGHT_RAW_MIN) {
        raw = LIGHT_RAW_MIN;
    }
#endif

    const uint32_t span = (uint32_t)(LIGHT_RAW_MAX - LIGHT_RAW_MIN);
    uint32_t pct = (uint32_t)(raw - LIGHT_RAW_MIN) * 100U / span;
#if LIGHT_ADC_INVERTED
    pct = 100U - pct;
#endif
    return (uint8_t)pct;
}

static esp_err_t oled_write(const uint8_t *data, size_t len)
{
    return i2c_master_write_to_device(I2C_PORT, OLED_I2C_ADDR, data, len, pdMS_TO_TICKS(OLED_I2C_TIMEOUT_MS));
}

static esp_err_t oled_cmd(uint8_t cmd)
{
    const uint8_t p[2] = {0x00, cmd};
    return oled_write(p, sizeof(p));
}

static esp_err_t oled_data(const uint8_t *data, size_t len)
{
    uint8_t buf[17] = {0};
    buf[0] = 0x40;

    size_t off = 0;
    while (off < len) {
        const size_t chunk = (len - off > 16U) ? 16U : (len - off);
        memcpy(&buf[1], &data[off], chunk);
        esp_err_t err = oled_write(buf, chunk + 1U);
        if (err != ESP_OK) {
            return err;
        }
        off += chunk;
    }

    return ESP_OK;
}

static void glyph_for(char c, uint8_t g[5])
{
    static const uint8_t digits[10][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
        {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
    };

    static const uint8_t upper[26][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
        {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
        {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x3A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
        {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
        {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
        {0x7F, 0x20, 0x18, 0x20, 0x7F}, {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
    };

    memset(g, 0, 5);

    if (c >= '0' && c <= '9') {
        memcpy(g, digits[c - '0'], 5);
        return;
    }

    if (c >= 'a' && c <= 'z') {
        c = (char)toupper((unsigned char)c);
    }

    if (c >= 'A' && c <= 'Z') {
        memcpy(g, upper[c - 'A'], 5);
        return;
    }

    switch (c) {
        case ' ': memset(g, 0x00, 5); break;
        case ':': g[1] = 0x36; g[3] = 0x36; break;
        case '.': g[2] = 0x60; break;
        case '-': g[1] = 0x08; g[2] = 0x08; g[3] = 0x08; break;
        case '%': g[0] = 0x63; g[1] = 0x13; g[2] = 0x08; g[3] = 0x64; g[4] = 0x63; break;
        case '/': g[4] = 0x03; g[3] = 0x0C; g[2] = 0x10; g[1] = 0x60; break;
        default:  g[0] = 0x7F; g[4] = 0x7F; break;
    }
}

static void oled_clear_fb(void)
{
    memset(s_fb, 0, sizeof(s_fb));
}

static void oled_draw_text(uint8_t page, uint8_t col, const char *text)
{
    if (page >= OLED_PAGES || col >= OLED_WIDTH || text == NULL) {
        return;
    }

    uint16_t x = col;
    while (*text != '\0' && (x + 6U) <= OLED_WIDTH) {
        uint8_t glyph[5];
        glyph_for(*text, glyph);

        const size_t idx = (size_t)page * OLED_WIDTH + x;
        memcpy(&s_fb[idx], glyph, 5);
        s_fb[idx + 5] = 0x00;

        x += 6;
        ++text;
    }
}

static esp_err_t oled_flush(void)
{
    for (uint8_t page = 0; page < OLED_PAGES; ++page) {
        ESP_RETURN_ON_ERROR(oled_cmd((uint8_t)(0xB0 | page)), TAG, "page cmd failed");
        ESP_RETURN_ON_ERROR(oled_cmd(0x00), TAG, "col low cmd failed");
        ESP_RETURN_ON_ERROR(oled_cmd(0x10), TAG, "col high cmd failed");

        const uint8_t *line = &s_fb[(size_t)page * OLED_WIDTH];
        ESP_RETURN_ON_ERROR(oled_data(line, OLED_WIDTH), TAG, "data write failed");
    }

    return ESP_OK;
}

esp_err_t oled_display_init(void)
{
    const uint8_t init_cmds[] = {
        0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10, 0x40,
        0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F, 0xA4, 0xD3,
        0x00, 0xD5, 0x80, 0xD9, 0xF1, 0xDA, 0x12, 0xDB,
        0x40, 0x8D, 0x14, 0xAF,
    };

    for (size_t i = 0; i < sizeof(init_cmds); ++i) {
        esp_err_t err = oled_cmd(init_cmds[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "init failed at cmd[%u]=0x%02X (%s)", (unsigned)i, init_cmds[i], esp_err_to_name(err));
            return err;
        }
    }

    oled_clear_fb();
    ESP_RETURN_ON_ERROR(oled_flush(), TAG, "flush after init failed");

    s_ready = true;
    ESP_LOGI(TAG, "ssd1306 init done");
    return ESP_OK;
}

esp_err_t oled_display_render(const system_state_t *snapshot)
{
    if (!s_ready || snapshot == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    char l1[22];
    char l2[22];
    char l3[22];
    char l4[22];

    const char *motion = snapshot->motion_detected ? "YES" : "NO";
    const char *light = snapshot->light_alert ? "ON" : "OFF";
    const uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    const bool in_reset_hold = now < snapshot->reset_hold_until_ms;
    const uint8_t light_pct = in_reset_hold ? 0U : light_percent_from_raw(snapshot->weighted_light_value);
    const char *status = "OK";
    if (in_reset_hold) {
        status = "RST";
    } else if (snapshot->status == SYSTEM_STATUS_WARN) {
        status = "WARN";
    } else if (snapshot->status == SYSTEM_STATUS_ALERT) {
        status = "ALERT";
    }

    snprintf(l1, sizeof(l1), "TEMP:%5.1fC", snapshot->temperature_c);
    snprintf(l2, sizeof(l2), "HUM :%5.1f%%", snapshot->humidity_pct);
    snprintf(l3, sizeof(l3), "M:%s L:%s %3u%%", motion, light, (unsigned)light_pct);
    snprintf(l4, sizeof(l4), "SYS:%s R:%lu", status, (unsigned long)snapshot->reset_count);

    oled_clear_fb();
    oled_draw_text(0, 0, l1);
    oled_draw_text(2, 0, l2);
    oled_draw_text(4, 0, l3);
    oled_draw_text(6, 0, l4);

    return oled_flush();
}

bool oled_display_is_ready(void)
{
    return s_ready;
}
