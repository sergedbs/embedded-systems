#ifndef ANALOG_MOTOR_H
#define ANALOG_MOTOR_H

#include <stdint.h>

#include "esp_err.h"

esp_err_t analog_motor_init(void);
esp_err_t analog_motor_set_speed_pct(int pct, uint32_t *duty_out);
int analog_motor_get_speed_pct(void);
uint32_t analog_motor_pct_to_duty(uint16_t pct);

#endif // ANALOG_MOTOR_H
