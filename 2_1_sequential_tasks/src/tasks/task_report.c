#include "task_report.h"
#include "esp_log.h"

static const char *TAG = "report";

void task_report_init(task_report_ctx_t *ctx, scheduler_t *sched, app_stats_t *stats) {
    ctx->sched = sched;
    ctx->stats = stats;
}

void task_report(void *ctx_void) {
    task_report_ctx_t *ctx = (task_report_ctx_t *)ctx_void;
    uint32_t avg = 0;
    bool has_avg = app_stats_average(ctx->stats, &avg);

    if (has_avg) {
        ESP_LOGI(TAG, "10s window: total=%lu short=%lu long=%lu avg_ms=%lu",
                 (unsigned long)ctx->stats->total_presses,
                 (unsigned long)ctx->stats->short_presses,
                 (unsigned long)ctx->stats->long_presses,
                 (unsigned long)avg);
    } else {
        ESP_LOGI(TAG, "10s window: total=%lu short=%lu long=%lu avg_ms=n/a",
                 (unsigned long)ctx->stats->total_presses,
                 (unsigned long)ctx->stats->short_presses,
                 (unsigned long)ctx->stats->long_presses);
    }

    app_stats_reset(ctx->stats);
}
