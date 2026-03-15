#ifndef MOTION_SENSOR_H
#define MOTION_SENSOR_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t motion_sensor_init(void);
bool motion_sensor_read_raw(void);

#endif // MOTION_SENSOR_H
