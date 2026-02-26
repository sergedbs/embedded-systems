#include "task_report.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "report";

void task_report_init(task_report_ctx_t *ctx,
                      app_stats_t *stats,
                      SemaphoreHandle_t data_mutex) {
    ctx->stats = stats;
    ctx->data_mutex = data_mutex;
}

void task_report(void *ctx_void) {
    task_report_ctx_t *ctx = (task_report_ctx_t *)ctx_void;
    TickType_t last_wake = xTaskGetTickCount();

    // small start offset to avoid collision with other tasks
    const TickType_t offset = pdMS_TO_TICKS(200);
    vTaskDelayUntil(&last_wake, offset);

    for (;;) {
        // wait 10 seconds between reports
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10000));

        uint32_t total = 0, short_cnt = 0, long_cnt = 0, avg = 0;
        bool has_avg = false;

        if (xSemaphoreTake(ctx->data_mutex, portMAX_DELAY) == pdTRUE) {
            total = ctx->stats->total_presses;
            short_cnt = ctx->stats->short_presses;
            long_cnt = ctx->stats->long_presses;
            has_avg = app_stats_average(ctx->stats, &avg);
            app_stats_reset(ctx->stats);
            xSemaphoreGive(ctx->data_mutex);
        }

        if (has_avg) {
            ESP_LOGI(TAG, "10s window: total=%lu short=%lu long=%lu avg_ms=%lu",
                     (unsigned long)total,
                     (unsigned long)short_cnt,
                     (unsigned long)long_cnt,
                     (unsigned long)avg);
        } else {
            ESP_LOGI(TAG, "10s window: total=%lu short=%lu long=%lu avg_ms=n/a",
                     (unsigned long)total,
                     (unsigned long)short_cnt,
                     (unsigned long)long_cnt);
        }
    }
}
