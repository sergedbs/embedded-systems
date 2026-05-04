#include "binary_actuator.h"

#include "app_config.h"
#include "driver/gpio.h"
#include "esp_check.h"

static const char *TAG = "binary_actuator";
static bool s_state;

static int output_level(bool on)
{
#if BINARY_ACTUATOR_ACTIVE_LOW
    return on ? 0 : 1;
#else
    return on ? 1 : 0;
#endif
}

esp_err_t binary_actuator_init(void)
{
    const gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_BINARY_ACTUATOR),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "gpio config failed");
    s_state = false;
    return binary_actuator_set_state(false);
}

esp_err_t binary_actuator_set_state(bool on)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_BINARY_ACTUATOR, output_level(on)), TAG, "set output failed");
    s_state = on;
    return ESP_OK;
}

bool binary_actuator_get_state(void)
{
    return s_state;
}
