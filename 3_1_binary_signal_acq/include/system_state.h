#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    float temperature_c;
    float humidity_pct;

    bool motion_detected;
    bool motion_stable;

    bool temp_alert;
    bool hum_alert;
    system_status_t status;

    uint32_t last_sensor_update_ms;

    uint32_t motion_events;
    uint32_t reset_count;
    uint32_t dht_error_count;
} system_state_t;

typedef struct {
    system_state_t state;
    SemaphoreHandle_t mutex;
} app_context_t;

void system_state_init(app_context_t *ctx);
void system_state_reset(app_context_t *ctx);
bool system_state_snapshot(app_context_t *ctx, system_state_t *out);

void system_state_set_dht(app_context_t *ctx, float temperature_c, float humidity_pct, uint32_t timestamp_ms, bool ok);
void system_state_set_motion(app_context_t *ctx, bool motion_detected, bool rising_edge);
void system_state_set_alerts(app_context_t *ctx, bool temp_alert, bool hum_alert, system_status_t status);

#endif // SYSTEM_STATE_H
