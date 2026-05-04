#include "task_output.h"

#include <math.h>
#include <stdint.h>

#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "heater/heater_output.h"

static const char *TAG = "task_output";

static bool pct_changed(float a, float b)
{
    return fabsf(a - b) >= 0.5f;
}

static void task_output_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    bool last_relay = false;
    float last_pwm_pct = -1.0f;
    bool has_output = false;

    while (true) {
        system_state_t s = {0};
        if (system_state_snapshot(ctx, &s)) {
            const bool relay_on = s.sensor_valid &&
                                  s.mode == CONTROL_MODE_ON_OFF &&
                                  s.relay_requested;
            const float pwm_pct = (s.sensor_valid && s.mode == CONTROL_MODE_PID) ? s.output_pct : 0.0f;
            const float applied_pct = (s.mode == CONTROL_MODE_ON_OFF && relay_on) ? 100.0f : pwm_pct;

            if (!has_output || relay_on != last_relay) {
                if (heater_output_set_relay(relay_on) != ESP_OK) {
                    ESP_LOGW(TAG, "relay update failed");
                }
                last_relay = relay_on;
            }

            uint32_t duty = s.pwm_duty;
            if (!has_output || pct_changed(pwm_pct, last_pwm_pct)) {
                if (heater_output_set_pwm_pct(pwm_pct, &duty) != ESP_OK) {
                    ESP_LOGW(TAG, "PWM update failed");
                }
                last_pwm_pct = pwm_pct;
            }

            system_state_set_actuator_output(ctx, relay_on, applied_pct, duty);
            has_output = true;
        }

        vTaskDelay(pdMS_TO_TICKS(ACTUATOR_OUTPUT_MS));
    }
}

bool task_output_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_output_fn,
                                      "TaskOutput",
                                      TASK_STACK_DEFAULT,
                                      ctx,
                                      TASK_PRIO_OUTPUT,
                                      NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskOutput");
        return false;
    }
    return true;
}
