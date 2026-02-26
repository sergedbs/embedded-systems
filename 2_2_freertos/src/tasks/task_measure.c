#include "task_measure.h"
#include "app_config.h"
#include "leds.h"
#include "driver/gpio.h"
#include "esp_timer.h"

static void handle_led_timeouts(task_measure_ctx_t *ctx, uint64_t now_us) {
    if (ctx->green_off_us && now_us >= ctx->green_off_us) {
        led_green_set(false);
        ctx->green_off_us = 0;
    }
    if (ctx->red_off_us && now_us >= ctx->red_off_us) {
        led_red_set(false);
        ctx->red_off_us = 0;
    }
}

void task_measure_init(task_measure_ctx_t *ctx,
                       press_event_t *shared_press,
                       SemaphoreHandle_t press_sem,
                       SemaphoreHandle_t data_mutex) {
    ctx->press_sem = press_sem;
    ctx->data_mutex = data_mutex;
    ctx->shared_press = shared_press;
    ctx->press_active = false;
    ctx->press_start_us = 0;
    ctx->green_off_us = 0;
    ctx->red_off_us = 0;

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
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(APP_TICK_MS));

        uint64_t now_us = esp_timer_get_time();
        handle_led_timeouts(ctx, now_us);

        button_event_t ev = {0};
        bool raw_level = gpio_get_level(APP_PIN_BTN);
        button_poll(&ctx->button, &ev, raw_level);

        if (ev.pressed_event) {
            ctx->press_start_us = now_us;
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

            if (xSemaphoreTake(ctx->data_mutex, portMAX_DELAY) == pdTRUE) {
                *ctx->shared_press = press_ev;
                xSemaphoreGive(ctx->data_mutex);
            }
            xSemaphoreGive(ctx->press_sem);

            if (press_ev.type == PRESS_SHORT) {
                led_green_set(true);
                led_red_set(false);
                ctx->green_off_us = now_us + (uint64_t)APP_LED_FLASH_MS * 1000ULL;
                ctx->red_off_us = 0;
            } else {
                led_red_set(true);
                led_green_set(false);
                ctx->red_off_us = now_us + (uint64_t)APP_LED_FLASH_MS * 1000ULL;
                ctx->green_off_us = 0;
            }
        }
    }
}
