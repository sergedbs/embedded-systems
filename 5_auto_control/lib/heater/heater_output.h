#ifndef HEATER_OUTPUT_H
#define HEATER_OUTPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t heater_output_init(void);
esp_err_t heater_output_set_relay(bool on);
esp_err_t heater_output_set_pwm_pct(float pct, uint32_t *duty_out);

bool heater_output_get_relay(void);
float heater_output_get_pwm_pct(void);
uint32_t heater_output_pct_to_duty(float pct);

#endif // HEATER_OUTPUT_H
