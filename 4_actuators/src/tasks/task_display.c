#include "task_display.h"

#include "app_config.h"
#include "display/lcd_display.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_display";

static const char *status_text(system_status_t status)
{
    switch (status) {
        case SYSTEM_STATUS_WARN:
            return "WARN";
        case SYSTEM_STATUS_ALERT:
            return "ALERT";
        case SYSTEM_STATUS_OK:
        default:
            return "OK";
    }
}

static bool snapshot_changed_for_report(const system_state_t *a, const system_state_t *b)
{
    return a->binary_output_state != b->binary_output_state ||
           a->binary_pending != b->binary_pending ||
           a->analog_raw_target_pct != b->analog_raw_target_pct ||
           a->analog_clamped_target_pct != b->analog_clamped_target_pct ||
           a->analog_weighted_target_pct != b->analog_weighted_target_pct ||
           a->analog_ramped_output_pct != b->analog_ramped_output_pct ||
           a->analog_pwm_duty != b->analog_pwm_duty ||
           a->analog_limit_reached != b->analog_limit_reached ||
           a->analog_ramping != b->analog_ramping ||
           a->command_error != b->command_error ||
           a->overload_simulated != b->overload_simulated ||
           a->command_count != b->command_count ||
           a->invalid_command_count != b->invalid_command_count ||
           a->status != b->status;
}

static void task_display_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    system_state_t s = {0};
    system_state_t last_report = {0};
    bool has_report = false;
    bool warned_no_display = false;

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            if (lcd_display_is_ready()) {
                (void)lcd_display_render(&s);
                warned_no_display = false;
            } else if (!warned_no_display) {
                ESP_LOGW(TAG, "LCD not ready, render skipped");
                warned_no_display = true;
            }

            if (!has_report || snapshot_changed_for_report(&s, &last_report)) {
                ESP_LOGI(TAG,
                         "BIN(raw/stable/out/pending)=%d/%d/%d/%d MOTOR(raw/clamp/med/w/ramp/duty)=%d/%d/%d/%d/%d/%lu flags(limit/ramp/err/ovl)=%d/%d/%d/%d cmds=%lu bad=%lu status=%s",
                         s.binary_raw_command,
                         s.binary_stable_command,
                         s.binary_output_state,
                         s.binary_pending,
                         s.analog_raw_target_pct,
                         s.analog_clamped_target_pct,
                         s.analog_median_target_pct,
                         s.analog_weighted_target_pct,
                         s.analog_ramped_output_pct,
                         (unsigned long)s.analog_pwm_duty,
                         s.analog_limit_reached,
                         s.analog_ramping,
                         s.command_error,
                         s.overload_simulated,
                         (unsigned long)s.command_count,
                         (unsigned long)s.invalid_command_count,
                         status_text(s.status));
                last_report = s;
                has_report = true;
            }
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
