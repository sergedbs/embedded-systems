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
    bool warned_no_display = false;

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            if (oled_display_is_ready()) {
                (void)oled_display_render(&s);
                warned_no_display = false;
            } else if (!warned_no_display) {
                ESP_LOGW(TAG, "OLED not ready, render skipped");
                warned_no_display = true;
            }
            ESP_LOGI(TAG, "T=%.1fC H=%.1f%% M=%d L=%u A(T/H/L)=%d/%d/%d R=%lu",
                     s.temperature_c,
                     s.humidity_pct,
                     s.motion_detected,
                     (unsigned)s.weighted_light_value,
                     s.temp_alert,
                     s.hum_alert,
                     s.light_alert,
                     (unsigned long)s.reset_count);
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
}

bool task_display_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_display_fn, "TaskDisplay", TASK_STACK_DEFAULT, ctx, TASK_PRIO_DISPLAY, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskDisplay");
        return false;
    }
    return true;
}
