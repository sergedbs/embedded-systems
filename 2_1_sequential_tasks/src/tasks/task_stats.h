#ifndef TASK_STATS_H
#define TASK_STATS_H

#include <stdint.h>
#include <stdbool.h>
#include "app_stats.h"
#include "app_types.h"
#include "scheduler.h"

typedef struct {
    scheduler_t *sched;
    event_queue_t *queue;
    app_stats_t *stats;
    uint32_t next_toggle_tick;
    uint8_t remaining_toggles;
    bool yellow_level;
} task_stats_ctx_t;

void task_stats_init(task_stats_ctx_t *ctx, scheduler_t *sched, event_queue_t *queue, app_stats_t *stats);
void task_stats(void *ctx_void);

#endif // TASK_STATS_H
