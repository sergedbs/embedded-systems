#include "motion_sensor.h"

#include "app_config.h"
#include "driver/gpio.h"

esp_err_t motion_sensor_init(void)
{
    gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_PIR_OUT),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    return gpio_config(&cfg);
}

bool motion_sensor_read_raw(void)
{
    return gpio_get_level(PIN_PIR_OUT) != 0;
}
