#include "app_stats.h"

void app_stats_reset(app_stats_t *s) {
    s->total_presses = 0;
    s->short_presses = 0;
    s->long_presses = 0;
    s->sum_total_ms = 0;
}

void app_stats_add(app_stats_t *s, const press_event_t *ev) {
    s->total_presses++;
    s->sum_total_ms += ev->duration_ms;
    if (ev->type == PRESS_SHORT) {
        s->short_presses++;
    } else {
        s->long_presses++;
    }
}

bool app_stats_average(const app_stats_t *s, uint32_t *avg_ms) {
    if (s->total_presses == 0) {
        return false;
    }
    *avg_ms = s->sum_total_ms / s->total_presses;
    return true;
}
