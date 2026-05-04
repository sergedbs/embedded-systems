#include "task_control.h"

#include "app_config.h"
#include "app_utils.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_control";

static bool compute_on_off_output(const system_state_t *s)
{
    const float lower = s->setpoint_c - s->hysteresis_c;
    const float upper = s->setpoint_c + s->hysteresis_c;

    if (s->filtered_temperature_c < lower) {
        return true;
    }
    if (s->filtered_temperature_c > upper) {
        return false;
    }
    return s->relay_requested;
}

static void task_control_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    float previous_error_c = 0.0f;

    while (true) {
        system_state_t s = {0};
        if (system_state_snapshot(ctx, &s) && s.sensor_valid) {
            const float dt_s = (float)CONTROL_PERIOD_MS / 1000.0f;
            const float error_c = s.setpoint_c - s.filtered_temperature_c;
            bool relay_requested = false;
            float output_pct = 0.0f;
            float p = 0.0f;
            float i = 0.0f;
            float d = 0.0f;
            float integral = s.pid_integral;

            if (s.mode == CONTROL_MODE_ON_OFF) {
                relay_requested = compute_on_off_output(&s);
                output_pct = relay_requested ? CONTROL_OUTPUT_MAX_PCT : CONTROL_OUTPUT_MIN_PCT;
                previous_error_c = error_c;
            } else {
                integral = app_clamp_float(integral + (error_c * dt_s), PID_INTEGRAL_MIN, PID_INTEGRAL_MAX);
                const float derivative = (error_c - previous_error_c) / dt_s;

                p = s.pid_kp * error_c;
                i = s.pid_ki * integral;
                d = s.pid_kd * derivative;
                output_pct = app_clamp_float(p + i + d, CONTROL_OUTPUT_MIN_PCT, CONTROL_OUTPUT_MAX_PCT);
                relay_requested = false;
                previous_error_c = error_c;
            }

            system_state_set_control_output(ctx, error_c, relay_requested, output_pct, p, i, d, integral);
        }

        vTaskDelay(pdMS_TO_TICKS(CONTROL_PERIOD_MS));
    }
}

bool task_control_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_control_fn,
                                      "TaskControl",
                                      TASK_STACK_DEFAULT,
                                      ctx,
                                      TASK_PRIO_CONTROL,
                                      NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskControl");
        return false;
    }
    return true;
}
