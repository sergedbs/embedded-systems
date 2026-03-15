#include "button_handler.h"

#include "app_config.h"
#include "driver/gpio.h"

esp_err_t button_handler_init(void)
{
    gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_BUTTON_RESET),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };

    return gpio_config(&cfg);
}

bool button_handler_poll_pressed(void)
{
    return gpio_get_level(PIN_BUTTON_RESET) == 0;
}
