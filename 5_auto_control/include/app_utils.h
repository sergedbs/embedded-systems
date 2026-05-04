#ifndef APP_UTILS_H
#define APP_UTILS_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static inline uint32_t app_now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static inline float app_clamp_float(float value, float min_value, float max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static inline bool app_float_is_valid(float value)
{
    return isfinite(value);
}

#endif // APP_UTILS_H
