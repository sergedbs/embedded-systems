#ifndef TASK_REPORT_H
#define TASK_REPORT_H

#include <stdint.h>
#include "app_stats.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    app_stats_t *stats;
    SemaphoreHandle_t data_mutex; // protect stats
} task_report_ctx_t;

void task_report_init(task_report_ctx_t *ctx,
                      app_stats_t *stats,
                      SemaphoreHandle_t data_mutex);

void task_report(void *ctx_void);

#endif // TASK_REPORT_H
