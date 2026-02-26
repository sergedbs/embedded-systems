#ifndef TASK_MEASURE_H
#define TASK_MEASURE_H

#include <stdbool.h>
#include <stdint.h>
#include "button.h"
#include "app_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    SemaphoreHandle_t press_sem;   // binary semaphore to signal Task2
    SemaphoreHandle_t data_mutex;  // protects shared_press (and stats in other tasks)
    press_event_t *shared_press;   // last measured press info (shared global)
    button_state_t button;
    bool press_active;
    uint64_t press_start_us;
    uint64_t green_off_us;
    uint64_t red_off_us;
} task_measure_ctx_t;

void task_measure_init(task_measure_ctx_t *ctx,
                       press_event_t *shared_press,
                       SemaphoreHandle_t press_sem,
                       SemaphoreHandle_t data_mutex);

void task_measure(void *ctx_void);

#endif // TASK_MEASURE_H
