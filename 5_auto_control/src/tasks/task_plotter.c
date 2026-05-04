#include "task_plotter.h"

#include <stdio.h>

#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_plotter";

static void task_plotter_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;

    while (true) {
        system_state_t s = {0};
        if (system_state_snapshot(ctx, &s) && s.plotter_enabled && s.sensor_valid) {
            printf("SetPoint:%.2f Value:%.2f Output:%.2f\n",
                   s.setpoint_c,
                   s.filtered_temperature_c,
                   s.output_pct);
            fflush(stdout);
        }

        vTaskDelay(pdMS_TO_TICKS(PLOTTER_PERIOD_MS));
    }
}

bool task_plotter_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_plotter_fn,
                                      "TaskPlotter",
                                      TASK_STACK_DEFAULT,
                                      ctx,
                                      TASK_PRIO_DISPLAY,
                                      NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskPlotter");
        return false;
    }
    return true;
}
