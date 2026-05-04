#ifndef BINARY_ACTUATOR_H
#define BINARY_ACTUATOR_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t binary_actuator_init(void);
esp_err_t binary_actuator_set_state(bool on);
bool binary_actuator_get_state(void);

#endif // BINARY_ACTUATOR_H
