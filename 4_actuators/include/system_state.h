#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    bool binary_raw_command;
    bool binary_stable_command;
    bool binary_output_state;
    bool binary_pending;
    uint32_t binary_command_ms;
    uint32_t binary_last_change_ms;

    int analog_raw_target_pct;
    int analog_clamped_target_pct;
    int analog_median_target_pct;
    int analog_weighted_target_pct;
    int analog_ramped_output_pct;
    uint32_t analog_pwm_duty;

    bool analog_limit_reached;
    bool analog_ramping;
    bool command_error;
    bool overload_simulated;
    system_status_t status;

    command_kind_t last_command_kind;
    uint32_t command_count;
    uint32_t invalid_command_count;
    uint32_t last_command_ms;
    uint32_t alert_page_until_ms;
} system_state_t;

typedef struct {
    system_state_t state;
    SemaphoreHandle_t mutex;
} app_context_t;

void system_state_init(app_context_t *ctx);
bool system_state_snapshot(app_context_t *ctx, system_state_t *out);

void system_state_set_binary_command(app_context_t *ctx, bool requested_on, uint32_t timestamp_ms);
void system_state_set_binary_conditioned(app_context_t *ctx, bool stable_on, bool pending, uint32_t timestamp_ms);
void system_state_set_binary_output(app_context_t *ctx, bool output_on);

void system_state_set_motor_command(app_context_t *ctx, int target_pct, uint32_t timestamp_ms, bool limit_reached);
void system_state_set_analog_conditioned(app_context_t *ctx,
                                         int clamped_pct,
                                         int median_pct,
                                         int weighted_pct,
                                         int ramped_pct,
                                         uint32_t pwm_duty,
                                         bool ramping,
                                         bool overload_simulated);

void system_state_record_status_request(app_context_t *ctx, uint32_t timestamp_ms);
void system_state_request_alert_page(app_context_t *ctx, uint32_t timestamp_ms, uint32_t duration_ms);
void system_state_record_invalid_command(app_context_t *ctx, uint32_t timestamp_ms);

#endif // SYSTEM_STATE_H
