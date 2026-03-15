#include "task_display.h"

#include "app_config.h"
#include "display/oled_display.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_display";

static void task_display_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    system_state_t s = {0};

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            (void)oled_display_render(&s);
            ESP_LOGI(TAG, "T=%.1fC H=%.1f%% M=%d A(T/H)=%d/%d R=%lu",
                     s.temperature_c,
                     s.humidity_pct,
                     s.motion_detected,
                     s.temp_alert,
                     s.hum_alert,
                     (unsigned long)s.reset_count);
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
}

void task_display_start(app_context_t *ctx)
{
    xTaskCreate(task_display_fn, "TaskDisplay", TASK_STACK_DEFAULT, ctx, TASK_PRIO_DISPLAY, NULL);
}
