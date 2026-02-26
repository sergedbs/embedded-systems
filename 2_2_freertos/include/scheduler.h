#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t recur_ticks;
    uint32_t offset_ticks;
    uint32_t next_run;
    void (*fn)(void *ctx);
    void *ctx;
} sched_task_t;

typedef struct {
    sched_task_t *tasks;
    size_t task_count;
    uint32_t tick;
} scheduler_t;

void scheduler_init(scheduler_t *s, sched_task_t *tasks, size_t count);
void scheduler_run_once(scheduler_t *s);
static inline uint32_t scheduler_current_tick(const scheduler_t *s) { return s->tick; }

#endif // SCHEDULER_H
