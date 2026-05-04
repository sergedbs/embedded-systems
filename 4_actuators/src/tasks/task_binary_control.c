#include "task_binary_control.h"

#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_binary_control";

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static void task_binary_control_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;

    while (true) {
        system_state_t s;
        if (system_state_snapshot(ctx, &s)) {
            const uint32_t now = now_ms();
            bool pending = s.binary_pending;
            bool stable = s.binary_stable_command;

            if (pending && (now - s.binary_command_ms) >= BINARY_DEBOUNCE_MS) {
                stable = s.binary_raw_command;
                pending = false;
                ESP_LOGI(TAG, "binary command stabilized: %s", stable ? "ON" : "OFF");
            }

            system_state_set_binary_conditioned(ctx, stable, pending, now);
        }

        vTaskDelay(pdMS_TO_TICKS(BINARY_CONTROL_MS));
    }
}

bool task_binary_control_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_binary_control_fn, "TaskBinary", TASK_STACK_DEFAULT, ctx, TASK_PRIO_BINARY, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskBinary");
        return false;
    }
    return true;
}
