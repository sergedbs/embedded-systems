#ifndef TASK_MEASURE_H
#define TASK_MEASURE_H

#include <stdint.h>
#include <stdbool.h>
#include "button.h"
#include "app_types.h"
#include "scheduler.h"

typedef struct {
    scheduler_t *sched;
    button_state_t button;
    event_queue_t *queue;
    uint64_t press_start_us;
    bool press_active;
    uint32_t green_off_tick;
    uint32_t red_off_tick;
} task_measure_ctx_t;

void task_measure_init(task_measure_ctx_t *ctx, scheduler_t *sched, event_queue_t *queue);
void task_measure(void *ctx_void);

#endif // TASK_MEASURE_H
