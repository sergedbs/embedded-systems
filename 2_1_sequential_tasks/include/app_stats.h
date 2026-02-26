#ifndef APP_STATS_H
#define APP_STATS_H

#include <stdint.h>
#include <stdbool.h>
#include "app_types.h"

typedef struct {
    uint32_t total_presses;
    uint32_t short_presses;
    uint32_t long_presses;
    uint32_t sum_total_ms;
} app_stats_t;

void app_stats_reset(app_stats_t *s);
void app_stats_add(app_stats_t *s, const press_event_t *ev);
bool app_stats_average(const app_stats_t *s, uint32_t *avg_ms);

#endif // APP_STATS_H
