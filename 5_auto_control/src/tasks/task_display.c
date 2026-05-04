#include "task_display.h"

#include "app_config.h"
#include "display/lcd_display.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_display";

static const char *mode_text(control_mode_t mode)
{
    return mode == CONTROL_MODE_PID ? "PID" : "ONOFF";
}

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

static bool report_changed(const system_state_t *a, const system_state_t *b)
{
    return a->mode != b->mode ||
           a->sensor_valid != b->sensor_valid ||
           a->relay_output != b->relay_output ||
           a->pwm_duty != b->pwm_duty ||
           a->command_count != b->command_count ||
           a->invalid_command_count != b->invalid_command_count ||
           a->status != b->status ||
           (int)(a->filtered_temperature_c * 10.0f) != (int)(b->filtered_temperature_c * 10.0f) ||
           (int)(a->setpoint_c * 10.0f) != (int)(b->setpoint_c * 10.0f) ||
           (int)(a->output_pct) != (int)(b->output_pct);
}

static void task_display_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    system_state_t s = {0};
    system_state_t last = {0};
    bool has_last = false;
    bool warned_no_lcd = false;

    while (true) {
        if (system_state_snapshot(ctx, &s)) {
            if (lcd_display_is_ready()) {
                (void)lcd_display_render(&s);
                warned_no_lcd = false;
            } else if (!warned_no_lcd) {
                ESP_LOGW(TAG, "LCD not ready, using serial report only");
                warned_no_lcd = true;
            }

            if (!has_last || report_changed(&s, &last)) {
                ESP_LOGI(TAG,
                         "mode=%s sp=%.2f temp=%.2f filt=%.2f hum=%.1f err=%.2f relay=%d out=%.1f duty=%lu pid(p/i/d)=%.1f/%.1f/%.1f sensor=%d cmds=%lu bad=%lu status=%s",
                         mode_text(s.mode),
                         s.setpoint_c,
                         s.temperature_c,
                         s.filtered_temperature_c,
                         s.humidity_pct,
                         s.error_c,
                         s.relay_output,
                         s.output_pct,
                         (unsigned long)s.pwm_duty,
                         s.pid_p,
                         s.pid_i,
                         s.pid_d,
                         s.sensor_valid,
                         (unsigned long)s.command_count,
                         (unsigned long)s.invalid_command_count,
                         status_text(s.status));
                last = s;
                has_last = true;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_REFRESH_MS));
    }
}

bool task_display_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_display_fn,
                                      "TaskDisplay",
                                      TASK_STACK_DEFAULT,
                                      ctx,
                                      TASK_PRIO_DISPLAY,
                                      NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskDisplay");
        return false;
    }
    return true;
}
