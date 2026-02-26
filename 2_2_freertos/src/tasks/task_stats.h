#ifndef TASK_STATS_H
#define TASK_STATS_H

#include <stdbool.h>
#include <stdint.h>
#include "app_stats.h"
#include "app_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    SemaphoreHandle_t press_sem;   // from Task1
    SemaphoreHandle_t data_mutex;  // protects shared_press and stats
    press_event_t *shared_press;
    app_stats_t *stats;
    uint32_t blink_remaining;
    TickType_t blink_next_tick;
    bool yellow_level;
} task_stats_ctx_t;

void task_stats_init(task_stats_ctx_t *ctx,
                     press_event_t *shared_press,
                     app_stats_t *stats,
                     SemaphoreHandle_t press_sem,
                     SemaphoreHandle_t data_mutex);

void task_stats(void *ctx_void);

#endif // TASK_STATS_H
