#include "system_state.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "system_state";

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
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
    ctx->state.last_sensor_update_ms = now_ms();
}

void system_state_reset(app_context_t *ctx)
{
    if (ctx == NULL || ctx->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE) {
        const uint32_t resets = ctx->state.reset_count + 1;
        memset(&ctx->state, 0, sizeof(ctx->state));
        ctx->state.status = SYSTEM_STATUS_OK;
        ctx->state.reset_count = resets;
        ctx->state.last_sensor_update_ms = now_ms();
        xSemaphoreGive(ctx->mutex);
    }
}

bool system_state_snapshot(app_context_t *ctx, system_state_t *out)
{
    if (ctx == NULL || out == NULL || ctx->mutex == NULL) {
        return false;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) != pdTRUE) {
        return false;
    }

    *out = ctx->state;
    xSemaphoreGive(ctx->mutex);
    return true;
}

void system_state_set_dht(app_context_t *ctx, float temperature_c, float humidity_pct, uint32_t timestamp_ms, bool ok)
{
    if (ctx == NULL || ctx->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE) {
        if (ok) {
            ctx->state.temperature_c = temperature_c;
            ctx->state.humidity_pct = humidity_pct;
            ctx->state.last_sensor_update_ms = timestamp_ms;
        } else {
            ctx->state.dht_error_count++;
        }
        xSemaphoreGive(ctx->mutex);
    }
}

void system_state_set_motion(app_context_t *ctx, bool motion_detected, bool rising_edge)
{
    if (ctx == NULL || ctx->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE) {
        ctx->state.motion_detected = motion_detected;
        ctx->state.motion_stable = motion_detected;
        if (rising_edge) {
            ctx->state.motion_events++;
        }
        xSemaphoreGive(ctx->mutex);
    }
}

void system_state_set_alerts(app_context_t *ctx, bool temp_alert, bool hum_alert, system_status_t status)
{
    if (ctx == NULL || ctx->mutex == NULL) {
        return;
    }

    if (xSemaphoreTake(ctx->mutex, portMAX_DELAY) == pdTRUE) {
        ctx->state.temp_alert = temp_alert;
        ctx->state.hum_alert = hum_alert;
        ctx->state.status = status;
        xSemaphoreGive(ctx->mutex);
    }
}
