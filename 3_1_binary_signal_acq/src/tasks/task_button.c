#include "task_button.h"

#include <stdbool.h>

#include "app_config.h"
#include "button/button_handler.h"
#include "freertos/task.h"

static void task_button_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    bool prev_pressed = false;

    while (true) {
        const bool pressed = button_handler_poll_pressed();

        if (pressed && !prev_pressed) {
            // Simple edge-triggered reset in iteration 2.
            system_state_reset(ctx);
        }

        prev_pressed = pressed;
        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

void task_button_start(app_context_t *ctx)
{
    xTaskCreate(task_button_fn, "TaskButton", TASK_STACK_DEFAULT, ctx, TASK_PRIO_BUTTON, NULL);
}
