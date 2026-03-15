#include "task_processing.h"

#include <stdint.h>

#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_processing";

static void task_processing_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    system_state_t s = {0};
    uint32_t last_seen_dht_update_ms = 0;
    uint8_t temp_high_confirm_count = 0;

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            bool temp_alert = s.temp_alert;
            bool hum_alert = s.hum_alert;
            const bool light_alert = s.light_alert;
            const bool light_led_on = s.light_led_on;
            const bool have_new_dht_sample = (s.last_sensor_update_ms != last_seen_dht_update_ms);

            if (temp_alert && s.temperature_c <= TEMP_ALERT_OFF_C) {
                temp_alert = false;
                temp_high_confirm_count = 0;
            } else if (!temp_alert && have_new_dht_sample) {
                if (s.temperature_c >= TEMP_ALERT_ON_C) {
                    if (temp_high_confirm_count < 255U) {
                        temp_high_confirm_count++;
                    }
                } else {
                    temp_high_confirm_count = 0;
                }

                if (temp_high_confirm_count >= TEMP_ALERT_ON_CONFIRM_SAMPLES) {
                    temp_alert = true;
                }
            }

            if (!hum_alert && s.humidity_pct >= HUM_ALERT_ON_PCT) {
                hum_alert = true;
            } else if (hum_alert && s.humidity_pct <= HUM_ALERT_OFF_PCT) {
                hum_alert = false;
            }

            if (have_new_dht_sample) {
                last_seen_dht_update_ms = s.last_sensor_update_ms;
            }

            system_status_t status = SYSTEM_STATUS_OK;
            if (temp_alert || hum_alert || light_alert) {
                status = SYSTEM_STATUS_ALERT;
            } else if (s.motion_detected) {
                status = SYSTEM_STATUS_WARN;
            }

            system_state_set_alerts(ctx, temp_alert, hum_alert, light_alert, light_led_on, status);
        }

        vTaskDelay(pdMS_TO_TICKS(PROCESSING_POLL_MS));
    }
}

bool task_processing_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_processing_fn, "TaskProcessing", TASK_STACK_DEFAULT, ctx, TASK_PRIO_PROCESS, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskProcessing");
        return false;
    }
    return true;
}
