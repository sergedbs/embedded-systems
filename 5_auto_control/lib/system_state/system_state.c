#include "system_state.h"

#include <string.h>

#include "app_config.h"
#include "app_utils.h"
#include "esp_log.h"

static const char *TAG = "system_state";

static bool lock_state(app_context_t *ctx)
{
    return ctx != NULL &&
           ctx->mutex != NULL &&
           xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE;
}

static void unlock_state(app_context_t *ctx)
{
    xSemaphoreGive(ctx->mutex);
}

static void refresh_status(system_state_t *s)
{
    if (s->command_error && (app_now_ms() - s->last_command_ms) >= COMMAND_ERROR_HOLD_MS) {
        s->command_error = false;
    }

    if (s->command_error || !s->sensor_valid) {
        s->status = SYSTEM_STATUS_ALERT;
    } else if (s->sensor_error_count > 0U) {
        s->status = SYSTEM_STATUS_WARN;
    } else {
        s->status = SYSTEM_STATUS_OK;
    }
}

void system_state_init(app_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->mutex = xSemaphoreCreateMutex();
    if (ctx->mutex == NULL) {
        ESP_LOGE(TAG, "failed to create mutex");
        return;
    }

    ctx->state.mode = CONTROL_MODE_ON_OFF;
    ctx->state.setpoint_c = CONTROL_SETPOINT_C;
    ctx->state.hysteresis_c = CONTROL_HYSTERESIS_C;
    ctx->state.pid_kp = PID_KP_DEFAULT;
    ctx->state.pid_ki = PID_KI_DEFAULT;
    ctx->state.pid_kd = PID_KD_DEFAULT;
    ctx->state.sensor_valid = false;
    ctx->state.plotter_enabled = false;
    ctx->state.status = SYSTEM_STATUS_ALERT;
    ctx->state.last_command_ms = app_now_ms();
}

bool system_state_snapshot(app_context_t *ctx, system_state_t *out)
{
    if (out == NULL || !lock_state(ctx)) {
        return false;
    }

    refresh_status(&ctx->state);
    *out = ctx->state;
    unlock_state(ctx);
    return true;
}

void system_state_set_sensor(app_context_t *ctx,
                             float temperature_c,
                             float humidity_pct,
                             uint32_t timestamp_ms,
                             bool valid)
{
    if (!lock_state(ctx)) {
        return;
    }

    if (valid) {
        if (!ctx->state.sensor_valid && ctx->state.sensor_read_count == 0U) {
            ctx->state.filtered_temperature_c = temperature_c;
        } else {
            ctx->state.filtered_temperature_c =
                (0.70f * ctx->state.filtered_temperature_c) + (0.30f * temperature_c);
        }
        ctx->state.temperature_c = temperature_c;
        ctx->state.humidity_pct = humidity_pct;
        ctx->state.sensor_read_count++;
        ctx->state.last_sensor_ms = timestamp_ms;
    } else {
        ctx->state.sensor_error_count++;
    }

    ctx->state.sensor_valid = valid;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_control_output(app_context_t *ctx,
                                     float error_c,
                                     bool relay_requested,
                                     float output_pct,
                                     float pid_p,
                                     float pid_i,
                                     float pid_d,
                                     float pid_integral)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.error_c = error_c;
    ctx->state.relay_requested = relay_requested;
    ctx->state.pid_p = pid_p;
    ctx->state.pid_i = pid_i;
    ctx->state.pid_d = pid_d;
    ctx->state.pid_integral = pid_integral;
    ctx->state.pid_output_pct = app_clamp_float(output_pct, CONTROL_OUTPUT_MIN_PCT, CONTROL_OUTPUT_MAX_PCT);
    ctx->state.output_pct = ctx->state.pid_output_pct;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_actuator_output(app_context_t *ctx,
                                      bool relay_output,
                                      float pwm_pct,
                                      uint32_t pwm_duty)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.relay_output = relay_output;
    ctx->state.output_pct = app_clamp_float(pwm_pct, CONTROL_OUTPUT_MIN_PCT, CONTROL_OUTPUT_MAX_PCT);
    ctx->state.pwm_duty = pwm_duty;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_mode(app_context_t *ctx, control_mode_t mode, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.mode = mode;
    ctx->state.pid_integral = 0.0f;
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_MODE;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_setpoint(app_context_t *ctx, float setpoint_c, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.setpoint_c = app_clamp_float(setpoint_c, DHT_TEMP_MIN_C, DHT_TEMP_MAX_C);
    ctx->state.pid_integral = 0.0f;
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_SETPOINT;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_hysteresis(app_context_t *ctx, float hysteresis_c, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.hysteresis_c = app_clamp_float(hysteresis_c, 0.1f, 10.0f);
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_HYSTERESIS;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_pid(app_context_t *ctx,
                          float kp,
                          float ki,
                          float kd,
                          uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.pid_kp = kp;
    ctx->state.pid_ki = ki;
    ctx->state.pid_kd = kd;
    ctx->state.pid_integral = 0.0f;
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_PID_GAIN;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_plotter(app_context_t *ctx, bool enabled, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.plotter_enabled = enabled;
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_PLOT;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_record_status_request(app_context_t *ctx, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_STATUS;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_record_invalid_command(app_context_t *ctx, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_INVALID;
    ctx->state.command_count++;
    ctx->state.invalid_command_count++;
    ctx->state.command_error = true;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}
