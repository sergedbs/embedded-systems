#include "task_analog_conditioning.h"

#include <stdint.h>

#include "actuators/analog_motor.h"
#include "app_config.h"
#include "esp_log.h"
#include "filters/filters.h"
#include "freertos/task.h"

static const char *TAG = "task_analog_conditioning";

static int clamp_int(int value, int min_value, int max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static int abs_int(int value)
{
    return (value < 0) ? -value : value;
}

static int ramp_toward(int current, int target)
{
    if (current < target) {
        const int next = current + MOTOR_RAMP_STEP_PCT;
        return (next > target) ? target : next;
    }
    if (current > target) {
        const int next = current - MOTOR_RAMP_STEP_PCT;
        return (next < target) ? target : next;
    }
    return current;
}

static void task_analog_conditioning_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    filter_median3_t median;
    filter_wma4_t weighted;
    int ramped = 0;
    bool was_overload = false;

    filter_median3_reset(&median);
    filter_wma4_reset(&weighted);

    while (true) {
        system_state_t s;
        if (system_state_snapshot(ctx, &s)) {
            const int clamped = clamp_int(s.analog_raw_target_pct, MOTOR_TARGET_MIN_PCT, MOTOR_TARGET_MAX_PCT);
            const int median_pct = filter_median3_apply(&median, clamped);
            const int weighted_pct = filter_wma4_apply(&weighted, median_pct);
            ramped = ramp_toward(ramped, weighted_pct);
            const bool ramping = (ramped != weighted_pct);
            const bool overload = (abs_int(ramped) >= MOTOR_OVERLOAD_PCT);
            const uint32_t duty = analog_motor_pct_to_duty((uint16_t)abs_int(ramped));

            system_state_set_analog_conditioned(ctx,
                                                clamped,
                                                median_pct,
                                                weighted_pct,
                                                ramped,
                                                duty,
                                                ramping,
                                                overload);
            if (overload && !was_overload) {
                ESP_LOGW(TAG, "simulated overload zone: ramped=%d%%", ramped);
            } else if (!overload && was_overload) {
                ESP_LOGI(TAG, "simulated overload cleared");
            }
            was_overload = overload;
        }

        vTaskDelay(pdMS_TO_TICKS(ANALOG_CONTROL_MS));
    }
}

bool task_analog_conditioning_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_analog_conditioning_fn, "TaskAnalog", TASK_STACK_DEFAULT, ctx, TASK_PRIO_ANALOG, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskAnalog");
        return false;
    }
    return true;
}
