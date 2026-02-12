#include "Led.h"

static gpio_num_t ledGpio;

void Led_Init(gpio_num_t gpio)
{
    ledGpio = gpio;
    gpio_reset_pin(ledGpio);
    gpio_set_direction(ledGpio, GPIO_MODE_OUTPUT);
    gpio_set_level(ledGpio, 0);
}

void Led_On(void)
{
    gpio_set_level(ledGpio, 1);
}

void Led_Off(void)
{
    gpio_set_level(ledGpio, 0);
}
