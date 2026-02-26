#include <stdio.h>
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "esp_task_wdt.h"
#include "scheduler.h"
#include "app_config.h"
#include "app_stats.h"
#include "leds.h"
#include "tasks/task_measure.h"
#include "tasks/task_stats.h"
#include "tasks/task_report.h"

// Simple non-preemptive loop: one task per tick.

void app_main(void) {
    leds_init();

    static scheduler_t sched;
    static event_queue_t queue;
    static app_stats_t stats;

    event_queue_init(&queue);
    app_stats_reset(&stats);

    // Register main task with WDT to avoid false triggers; keep timeout generous.
    const esp_task_wdt_config_t wdt_cfg = {
        .timeout_ms = 10000,    // 10 s
        .idle_core_mask = 0,    // no idle tasks
        .trigger_panic = false,
    };
    esp_task_wdt_init(&wdt_cfg);
    esp_task_wdt_add(NULL);

    static task_measure_ctx_t measure_ctx;
    static task_stats_ctx_t stats_ctx;
    static task_report_ctx_t report_ctx;

    task_measure_init(&measure_ctx, &sched, &queue);
    task_stats_init(&stats_ctx, &sched, &queue, &stats);
    task_report_init(&report_ctx, &sched, &stats);

    static sched_task_t tasks[] = {
        // Stagger offsets so each task gets time; one task runs per tick.
        {.recur_ticks = 1, .offset_ticks = 0, .fn = task_measure, .ctx = &measure_ctx},
        {.recur_ticks = 1, .offset_ticks = 1, .fn = task_stats, .ctx = &stats_ctx},
        {.recur_ticks = APP_REPORT_INTERVAL_TICKS, .offset_ticks = 5, .fn = task_report, .ctx = &report_ctx},
    };

    scheduler_init(&sched, tasks, sizeof(tasks) / sizeof(tasks[0]));

    uint64_t next_tick_us = esp_timer_get_time();
    while (1) {
        uint64_t now = esp_timer_get_time();
        if (now >= next_tick_us) {
            scheduler_run_once(&sched);
            next_tick_us += APP_TICK_US;
            esp_task_wdt_reset();
        } else {
            // light wait to avoid spinning too hard
            uint64_t remain = next_tick_us - now;
            if (remain > 0) {
                if (remain > 10000) {
                    remain = 10000; // cap to 10ms to avoid WDT issues
                }
                esp_rom_delay_us((uint32_t)remain);
            }
        }
    }
}
