#include "task_processing.h"

#include "app_config.h"
#include "freertos/task.h"

static void task_processing_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    system_state_t s = {0};

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            bool temp_alert = s.temp_alert;
            bool hum_alert = s.hum_alert;

            if (!temp_alert && s.temperature_c >= TEMP_ALERT_ON_C) {
                temp_alert = true;
            } else if (temp_alert && s.temperature_c <= TEMP_ALERT_OFF_C) {
                temp_alert = false;
            }

            if (!hum_alert && s.humidity_pct >= HUM_ALERT_ON_PCT) {
                hum_alert = true;
            } else if (hum_alert && s.humidity_pct <= HUM_ALERT_OFF_PCT) {
                hum_alert = false;
            }

            system_status_t status = SYSTEM_STATUS_OK;
            if (temp_alert || hum_alert) {
                status = SYSTEM_STATUS_ALERT;
            } else if (s.motion_detected) {
                status = SYSTEM_STATUS_WARN;
            }

            system_state_set_alerts(ctx, temp_alert, hum_alert, status);
        }

        vTaskDelay(pdMS_TO_TICKS(PROCESSING_POLL_MS));
    }
}

void task_processing_start(app_context_t *ctx)
{
    xTaskCreate(task_processing_fn, "TaskProcessing", TASK_STACK_DEFAULT, ctx, TASK_PRIO_PROCESS, NULL);
}
