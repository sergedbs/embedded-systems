#include "task_measure.h"
#include "leds.h"
#include "app_config.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static void handle_led_timeouts(task_measure_ctx_t *ctx, uint32_t tick_now) {
    if (ctx->green_off_tick && tick_now >= ctx->green_off_tick) {
        led_green_set(false);
        ctx->green_off_tick = 0;
    }
    if (ctx->red_off_tick && tick_now >= ctx->red_off_tick) {
        led_red_set(false);
        ctx->red_off_tick = 0;
    }
}

void task_measure_init(task_measure_ctx_t *ctx, scheduler_t *sched, event_queue_t *queue) {
    ctx->sched = sched;
    ctx->queue = queue;
    ctx->press_start_us = 0;
    ctx->press_active = false;
    ctx->green_off_tick = 0;
    ctx->red_off_tick = 0;

    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << APP_PIN_BTN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&btn_cfg);

    bool raw_level = gpio_get_level(APP_PIN_BTN);
    button_init(&ctx->button, raw_level);
}

void task_measure(void *ctx_void) {
    task_measure_ctx_t *ctx = (task_measure_ctx_t *)ctx_void;
    uint32_t tick_now = scheduler_current_tick(ctx->sched);

    handle_led_timeouts(ctx, tick_now);

    button_event_t ev = {0};
    bool raw_level = gpio_get_level(APP_PIN_BTN);
    button_poll(&ctx->button, &ev, raw_level);

    if (ev.pressed_event) {
        ctx->press_start_us = esp_timer_get_time();
        ctx->press_active = true;
    }

    if (ev.released_event && ctx->press_active) {
        uint64_t release_us = esp_timer_get_time();
        uint32_t duration_ms = (uint32_t)((release_us - ctx->press_start_us) / 1000ULL);
        ctx->press_active = false;

        press_event_t press_ev = {
            .duration_ms = duration_ms,
            .type = (duration_ms < APP_LONG_PRESS_MS) ? PRESS_SHORT : PRESS_LONG
        };
        (void)event_queue_push(ctx->queue, &press_ev);

        // brief feedback via red/green LEDs
        if (press_ev.type == PRESS_SHORT) {
            led_green_set(true);
            led_red_set(false);
            ctx->green_off_tick = tick_now + APP_LED_FLASH_TICKS;
            ctx->red_off_tick = 0;
        } else {
            led_red_set(true);
            led_green_set(false);
            ctx->red_off_tick = tick_now + APP_LED_FLASH_TICKS;
            ctx->green_off_tick = 0;
        }
    }
}
