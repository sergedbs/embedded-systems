#include "led.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED";

static const gpio_num_t led_pins[] = {
    [LED_GREEN] = GPIO_LED_GREEN,
    [LED_RED]   = GPIO_LED_RED,
};

#define LED_COUNT (sizeof(led_pins) / sizeof(led_pins[0]))

esp_err_t led_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_LED_GREEN) | (1ULL << GPIO_LED_RED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED pins: %s", esp_err_to_name(ret));
        return ret;
    }

    led_all_off();
    ESP_LOGI(TAG, "LEDs initialized");
    return ESP_OK;
}

void led_set(led_color_t color, bool state)
{
    if ((size_t)color >= LED_COUNT) {
        ESP_LOGW(TAG, "Invalid LED color: %d", color);
        return;
    }
    gpio_set_level(led_pins[color], state ? 1 : 0);
}

void led_all_off(void)
{
    for (size_t i = 0; i < LED_COUNT; i++) {
        gpio_set_level(led_pins[i], 0);
    }
}

void led_success(void)
{
    led_set(LED_GREEN, true);
    led_set(LED_RED,   false);
}

void led_error(void)
{
    led_set(LED_GREEN, false);
    led_set(LED_RED,   true);
}
