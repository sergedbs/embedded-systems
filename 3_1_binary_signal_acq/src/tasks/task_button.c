#include "task_button.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "button/button_handler.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_button";

static void task_button_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    bool stable_pressed = false;
    bool candidate_pressed = false;
    uint32_t stable_for_ms = 0;
    bool reset_fired_for_press = false;

    while (true) {
        const bool raw_pressed = button_handler_poll_pressed();

        if (raw_pressed == candidate_pressed) {
            stable_for_ms += BUTTON_POLL_MS;
        } else {
            candidate_pressed = raw_pressed;
            stable_for_ms = BUTTON_POLL_MS;
        }

        if (candidate_pressed != stable_pressed && stable_for_ms >= BUTTON_DEBOUNCE_MS) {
            stable_pressed = candidate_pressed;

            if (stable_pressed) {
                if (!reset_fired_for_press) {
                    system_state_reset(ctx);
                    reset_fired_for_press = true;
                }
            } else {
                reset_fired_for_press = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_MS));
    }
}

bool task_button_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_button_fn, "TaskButton", TASK_STACK_DEFAULT, ctx, TASK_PRIO_BUTTON, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskButton");
        return false;
    }
    return true;
}
