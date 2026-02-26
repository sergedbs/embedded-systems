#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "leds.h"
#include "app_config.h"
#include "app_stats.h"
#include "tasks/task_measure.h"
#include "tasks/task_stats.h"
#include "tasks/task_report.h"

// Shared resources protected by mutex
press_event_t g_last_press;
app_stats_t g_stats;

SemaphoreHandle_t g_press_sem;
SemaphoreHandle_t g_data_mutex;

void app_main(void) {
    leds_init();
    app_stats_reset(&g_stats);

    g_press_sem = xSemaphoreCreateBinary();
    g_data_mutex = xSemaphoreCreateMutex();

    static task_measure_ctx_t measure_ctx;
    static task_stats_ctx_t stats_ctx;
    static task_report_ctx_t report_ctx;

    task_measure_init(&measure_ctx, &g_last_press, g_press_sem, g_data_mutex);
    task_stats_init(&stats_ctx, &g_last_press, &g_stats, g_press_sem, g_data_mutex);
    task_report_init(&report_ctx, &g_stats, g_data_mutex);

    xTaskCreate(task_measure, "measure", 4096, &measure_ctx, 3, NULL);
    xTaskCreate(task_stats,   "stats",   4096, &stats_ctx,   2, NULL);
    xTaskCreate(task_report,  "report",  4096, &report_ctx,  1, NULL);
}
