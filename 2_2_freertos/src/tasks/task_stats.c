#include "task_stats.h"
#include "app_config.h"
#include "leds.h"

static void start_blink(task_stats_ctx_t *ctx, uint8_t blinks, TickType_t now_tick) {
    ctx->blink_remaining = (uint32_t)blinks * 2U; // on/off pairs
    ctx->blink_next_tick = now_tick;
    ctx->yellow_level = led_yellow_get();
}

static void service_blink(task_stats_ctx_t *ctx, TickType_t now_tick) {
    if (ctx->blink_remaining == 0) {
        return;
    }
    if ((int32_t)(now_tick - ctx->blink_next_tick) < 0) {
        return;
    }
    ctx->yellow_level = !ctx->yellow_level;
    led_yellow_set(ctx->yellow_level);
    ctx->blink_remaining--;
    ctx->blink_next_tick = now_tick + pdMS_TO_TICKS(APP_BLINK_INTERVAL_MS);

    if (ctx->blink_remaining == 0) {
        led_yellow_set(false);
        ctx->yellow_level = false;
    }
}

void task_stats_init(task_stats_ctx_t *ctx,
                     press_event_t *shared_press,
                     app_stats_t *stats,
                     SemaphoreHandle_t press_sem,
                     SemaphoreHandle_t data_mutex) {
    ctx->shared_press = shared_press;
    ctx->stats = stats;
    ctx->press_sem = press_sem;
    ctx->data_mutex = data_mutex;
    ctx->blink_remaining = 0;
    ctx->blink_next_tick = 0;
    ctx->yellow_level = false;
}

void task_stats(void *ctx_void) {
    task_stats_ctx_t *ctx = (task_stats_ctx_t *)ctx_void;

    const TickType_t wait_slice = pdMS_TO_TICKS(APP_BLINK_INTERVAL_MS / 2);

    for (;;) {
        // Periodically service blink while waiting for semaphore
        service_blink(ctx, xTaskGetTickCount());

        if (xSemaphoreTake(ctx->press_sem, wait_slice) == pdTRUE) {
            // Copy shared press data under mutex
            press_event_t ev;
            if (xSemaphoreTake(ctx->data_mutex, portMAX_DELAY) == pdTRUE) {
                ev = *ctx->shared_press;
                xSemaphoreGive(ctx->data_mutex);
            } else {
                continue; // shouldn't happen
            }

            // Update stats under same mutex to reuse protection of shared resource
            if (xSemaphoreTake(ctx->data_mutex, portMAX_DELAY) == pdTRUE) {
                app_stats_add(ctx->stats, &ev);
                xSemaphoreGive(ctx->data_mutex);
            }

            uint8_t blinks = (ev.type == PRESS_SHORT) ? 5U : 10U;
            start_blink(ctx, blinks, xTaskGetTickCount());
        }
    }
}
