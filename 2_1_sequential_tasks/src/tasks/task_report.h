#ifndef TASK_REPORT_H
#define TASK_REPORT_H

#include "scheduler.h"
#include "app_stats.h"

typedef struct {
    scheduler_t *sched;
    app_stats_t *stats;
} task_report_ctx_t;

void task_report_init(task_report_ctx_t *ctx, scheduler_t *sched, app_stats_t *stats);
void task_report(void *ctx_void);

#endif // TASK_REPORT_H
