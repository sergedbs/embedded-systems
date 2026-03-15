#include "task_motion.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "motion/motion_sensor.h"

static const char *TAG = "task_motion";

static void task_motion_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    bool stable_motion = motion_sensor_read_raw();
    bool candidate_motion = stable_motion;
    uint8_t candidate_count = 0;

    gpio_set_level(PIN_LED_MOTION, stable_motion ? 1 : 0);
    system_state_set_motion(ctx, stable_motion, false);

    while (true) {
        const bool raw_motion = motion_sensor_read_raw();

        if (raw_motion != stable_motion) {
            if (raw_motion == candidate_motion) {
                if (candidate_count < UINT8_MAX) {
                    candidate_count++;
                }
            } else {
                candidate_motion = raw_motion;
                candidate_count = 1;
            }

            if (candidate_count >= MOTION_STABLE_SAMPLES) {
                const bool rising = (!stable_motion && candidate_motion);
                stable_motion = candidate_motion;
                candidate_count = 0;

                gpio_set_level(PIN_LED_MOTION, stable_motion ? 1 : 0);
                system_state_set_motion(ctx, stable_motion, rising);
            }
        } else {
            candidate_motion = stable_motion;
            candidate_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(MOTION_POLL_MS));
    }
}

bool task_motion_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_motion_fn, "TaskMotion", TASK_STACK_DEFAULT, ctx, TASK_PRIO_MOTION, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskMotion");
        return false;
    }
    return true;
}
