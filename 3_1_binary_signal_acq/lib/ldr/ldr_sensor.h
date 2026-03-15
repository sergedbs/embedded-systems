#ifndef LDR_SENSOR_H
#define LDR_SENSOR_H

#include <stdint.h>

#include "esp_err.h"

esp_err_t ldr_sensor_init(void);
esp_err_t ldr_sensor_read_raw(uint16_t *value);

#endif // LDR_SENSOR_H
