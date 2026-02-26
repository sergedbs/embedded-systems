#include "leds.h"
#include "driver/gpio.h"

void leds_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << APP_PIN_LED_GREEN) |
                        (1ULL << APP_PIN_LED_RED) |
                        (1ULL << APP_PIN_LED_YELLOW),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);
    led_green_set(false);
    led_red_set(false);
    led_yellow_set(false);
}

void led_green_set(bool on) { gpio_set_level(APP_PIN_LED_GREEN, on); }
void led_red_set(bool on) { gpio_set_level(APP_PIN_LED_RED, on); }
void led_yellow_set(bool on) { gpio_set_level(APP_PIN_LED_YELLOW, on); }
bool led_yellow_get(void) { return gpio_get_level(APP_PIN_LED_YELLOW); }
