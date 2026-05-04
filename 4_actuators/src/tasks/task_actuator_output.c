#include "task_actuator_output.h"

#include <stdint.h>
#include <limits.h>

#include "actuators/analog_motor.h"
#include "actuators/binary_actuator.h"
#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_actuator_output";

static void task_actuator_output_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    bool binary_applied = false;
    bool last_binary = false;
    int last_motor_pct = INT_MIN;

    while (true) {
        system_state_t s;
        if (system_state_snapshot(ctx, &s)) {
            if ((!binary_applied || s.binary_stable_command != last_binary) &&
                binary_actuator_set_state(s.binary_stable_command) == ESP_OK) {
                system_state_set_binary_output(ctx, s.binary_stable_command);
                last_binary = s.binary_stable_command;
                binary_applied = true;
            }

            if (s.analog_ramped_output_pct != last_motor_pct &&
                analog_motor_set_speed_pct(s.analog_ramped_output_pct, NULL) != ESP_OK) {
                ESP_LOGW(TAG, "failed to update motor output");
            } else {
                last_motor_pct = s.analog_ramped_output_pct;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(ACTUATOR_OUTPUT_MS));
    }
}

bool task_actuator_output_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_actuator_output_fn, "TaskOutput", TASK_STACK_DEFAULT, ctx, TASK_PRIO_OUTPUT, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskOutput");
        return false;
    }
    return true;
}
