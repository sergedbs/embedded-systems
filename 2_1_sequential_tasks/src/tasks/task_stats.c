#include "task_stats.h"
#include "app_config.h"
#include "leds.h"

void task_stats_init(task_stats_ctx_t *ctx, scheduler_t *sched, event_queue_t *queue, app_stats_t *stats) {
    ctx->sched = sched;
    ctx->queue = queue;
    ctx->stats = stats;
    ctx->next_toggle_tick = 0;
    ctx->remaining_toggles = 0;
    ctx->yellow_level = false;
}

static void start_blink(task_stats_ctx_t *ctx, uint8_t blinks, uint32_t tick_now) {
    ctx->remaining_toggles = (uint8_t)(blinks * 2U); // on/off pairs
    ctx->next_toggle_tick = tick_now;
    ctx->yellow_level = led_yellow_get();
}

static void service_blink(task_stats_ctx_t *ctx, uint32_t tick_now) {
    if (ctx->remaining_toggles == 0) {
        return;
    }
    if (tick_now < ctx->next_toggle_tick) {
        return;
    }
    ctx->yellow_level = !ctx->yellow_level;
    led_yellow_set(ctx->yellow_level);
    ctx->remaining_toggles--;
    ctx->next_toggle_tick = tick_now + APP_BLINK_INTERVAL_TICKS;

    if (ctx->remaining_toggles == 0) {
        // ensure LED ends off
        led_yellow_set(false);
        ctx->yellow_level = false;
    }
}

void task_stats(void *ctx_void) {
    task_stats_ctx_t *ctx = (task_stats_ctx_t *)ctx_void;
    uint32_t tick_now = scheduler_current_tick(ctx->sched);

    press_event_t ev;
    if (event_queue_pop(ctx->queue, &ev)) {
        app_stats_add(ctx->stats, &ev);
        uint8_t blinks = (ev.type == PRESS_SHORT) ? 5U : 10U;
        start_blink(ctx, blinks, tick_now);
    }

    service_blink(ctx, tick_now);
}
