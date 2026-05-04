#include "system_state.h"

#include <string.h>

#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "system_state";

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

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

static system_status_t derive_status(const system_state_t *s)
{
    if (s->command_error || s->overload_simulated) {
        return SYSTEM_STATUS_ALERT;
    }
    if (s->analog_limit_reached || s->analog_ramping || s->binary_pending) {
        return SYSTEM_STATUS_WARN;
    }
    return SYSTEM_STATUS_OK;
}

static void refresh_status(system_state_t *s)
{
    if (s->command_error && (now_ms() - s->last_command_ms) >= COMMAND_ERROR_HOLD_MS) {
        s->command_error = false;
    }
    s->status = derive_status(s);
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

    ctx->state.status = SYSTEM_STATUS_OK;
    ctx->state.last_command_ms = now_ms();
}

bool system_state_snapshot(app_context_t *ctx, system_state_t *out)
{
    if (out == NULL || !lock_state(ctx)) {
        return false;
    }

    *out = ctx->state;
    unlock_state(ctx);
    return true;
}

void system_state_set_binary_command(app_context_t *ctx, bool requested_on, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.binary_raw_command = requested_on;
    ctx->state.binary_pending = true;
    ctx->state.binary_command_ms = timestamp_ms;
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_BIN;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_binary_conditioned(app_context_t *ctx, bool stable_on, bool pending, uint32_t timestamp_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    const bool changed = (ctx->state.binary_stable_command != stable_on);
    ctx->state.binary_stable_command = stable_on;
    ctx->state.binary_pending = pending;
    if (changed) {
        ctx->state.binary_last_change_ms = timestamp_ms;
    }
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_binary_output(app_context_t *ctx, bool output_on)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.binary_output_state = output_on;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_motor_command(app_context_t *ctx, int target_pct, uint32_t timestamp_ms, bool limit_reached)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.analog_raw_target_pct = target_pct;
    ctx->state.analog_limit_reached = limit_reached;
    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = (target_pct == 0) ? COMMAND_KIND_STOP : COMMAND_KIND_MOTOR;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    refresh_status(&ctx->state);
    unlock_state(ctx);
}

void system_state_set_analog_conditioned(app_context_t *ctx,
                                         int clamped_pct,
                                         int median_pct,
                                         int weighted_pct,
                                         int ramped_pct,
                                         uint32_t pwm_duty,
                                         bool ramping,
                                         bool overload_simulated)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.analog_clamped_target_pct = clamped_pct;
    ctx->state.analog_median_target_pct = median_pct;
    ctx->state.analog_weighted_target_pct = weighted_pct;
    ctx->state.analog_ramped_output_pct = ramped_pct;
    ctx->state.analog_pwm_duty = pwm_duty;
    ctx->state.analog_ramping = ramping;
    ctx->state.overload_simulated = overload_simulated;
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

void system_state_request_alert_page(app_context_t *ctx, uint32_t timestamp_ms, uint32_t duration_ms)
{
    if (!lock_state(ctx)) {
        return;
    }

    ctx->state.last_command_ms = timestamp_ms;
    ctx->state.last_command_kind = COMMAND_KIND_ALERT;
    ctx->state.command_count++;
    ctx->state.command_error = false;
    ctx->state.alert_page_until_ms = timestamp_ms + duration_ms;
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
