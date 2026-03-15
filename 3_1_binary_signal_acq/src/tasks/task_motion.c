#include "task_motion.h"

#include <stdbool.h>

#include "app_config.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "motion/motion_sensor.h"

static void task_motion_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    bool prev = false;

    while (true) {
        const bool motion = motion_sensor_read_raw();
        const bool rising = (!prev && motion);

        gpio_set_level(PIN_LED_MOTION, motion ? 1 : 0);
        system_state_set_motion(ctx, motion, rising);

        prev = motion;
        vTaskDelay(pdMS_TO_TICKS(MOTION_POLL_MS));
    }
}

void task_motion_start(app_context_t *ctx)
{
    xTaskCreate(task_motion_fn, "TaskMotion", TASK_STACK_DEFAULT, ctx, TASK_PRIO_MOTION, NULL);
}
