#include "scheduler.h"
#include <stdbool.h>

void scheduler_init(scheduler_t *s, sched_task_t *tasks, size_t count) {
    s->tasks = tasks;
    s->task_count = count;
    s->tick = 0;
    for (size_t i = 0; i < count; ++i) {
        s->tasks[i].next_run = s->tasks[i].offset_ticks;
    }
}

void scheduler_run_once(scheduler_t *s) {
    uint32_t tick = s->tick;
    // Run all tasks that are due for this tick, in table order.
    // Still non-preemptive: tasks run sequentially within the tick.
    while (1) {
        bool ran = false;
        for (size_t i = 0; i < s->task_count; ++i) {
            if (tick >= s->tasks[i].next_run) {
                if (s->tasks[i].fn) {
                    s->tasks[i].fn(s->tasks[i].ctx);
                }
                s->tasks[i].next_run += s->tasks[i].recur_ticks;
                ran = true;
                break; // run only one and re-scan to preserve order
            }
        }
        if (!ran) {
            break;
        }
    }
    s->tick++;
}
