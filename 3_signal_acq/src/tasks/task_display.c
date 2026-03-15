#include "task_display.h"

#include "app_config.h"
#include "display/oled_display.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_display";

static uint8_t light_percent_from_raw(uint16_t raw)
{
    if (LIGHT_RAW_MAX <= LIGHT_RAW_MIN) {
        return 0;
    }

    if (raw > LIGHT_RAW_MAX) {
        raw = LIGHT_RAW_MAX;
    }
#if LIGHT_RAW_MIN > 0
    else if (raw < LIGHT_RAW_MIN) {
        raw = LIGHT_RAW_MIN;
    }
#endif

    const uint32_t span = (uint32_t)(LIGHT_RAW_MAX - LIGHT_RAW_MIN);
    uint32_t pct = (uint32_t)(raw - LIGHT_RAW_MIN) * 100U / span;
#if LIGHT_ADC_INVERTED
    pct = 100U - pct;
#endif
    return (uint8_t)pct;
}

static void task_display_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    system_state_t s = {0};
    bool warned_no_display = false;

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            const uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            const int in_reset_hold = (now < s.reset_hold_until_ms) ? 1 : 0;
            const uint8_t light_pct = in_reset_hold ? 0U : light_percent_from_raw(s.weighted_light_value);
            if (oled_display_is_ready()) {
                (void)oled_display_render(&s);
                warned_no_display = false;
            } else if (!warned_no_display) {
                ESP_LOGW(TAG, "OLED not ready, render skipped");
                warned_no_display = true;
            }
            ESP_LOGI(TAG, "T=%.1fC H=%.1f%% M=%d Lraw=%u Lpct=%u%% A(T/H/L)=%d/%d/%d R=%lu hold=%d",
                     s.temperature_c,
                     s.humidity_pct,
                     s.motion_detected,
                     (unsigned)s.weighted_light_value,
                     (unsigned)light_pct,
                     s.temp_alert,
                     s.hum_alert,
                     s.light_alert,
                     (unsigned long)s.reset_count,
                     in_reset_hold);
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
