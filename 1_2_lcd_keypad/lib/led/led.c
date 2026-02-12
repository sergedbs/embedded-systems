#include "led.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "LED";

// LED GPIO array for easy access
static const gpio_num_t led_pins[] = {
    GPIO_LED_GREEN,
    GPIO_LED_RED
};

esp_err_t led_init(void)
{
    esp_err_t ret;
    
    // Configure LED pins as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_LED_GREEN) | (1ULL << GPIO_LED_RED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Turn off all LEDs initially
    led_all_off();
    
    ESP_LOGI(TAG, "LEDs initialized successfully");
    return ESP_OK;
}

void led_set(led_color_t color, bool state)
{
    if (color >= sizeof(led_pins) / sizeof(led_pins[0])) {
        ESP_LOGW(TAG, "Invalid LED color: %d", color);
        return;
    }
    
    gpio_set_level(led_pins[color], state ? 1 : 0);
    ESP_LOGD(TAG, "LED %s: %s", 
             color == LED_GREEN ? "GREEN" : "RED",
             state ? "ON" : "OFF");
}

void led_all_off(void)
{
    gpio_set_level(GPIO_LED_GREEN, 0);
    gpio_set_level(GPIO_LED_RED, 0);
    ESP_LOGD(TAG, "All LEDs turned off");
}

void led_success(void)
{
    led_set(LED_GREEN, true);
    led_set(LED_RED, false);
}

void led_error(void)
{
    led_set(LED_GREEN, false);
    led_set(LED_RED, true);
}
