#ifndef LED_H
#define LED_H

#include "driver/gpio.h"

void Led_Init(gpio_num_t gpio);
void Led_On(void);
void Led_Off(void);

#endif
