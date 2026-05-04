#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

typedef struct {
    control_mode_t mode;
    float setpoint_c;
    float hysteresis_c;

    float temperature_c;
    float filtered_temperature_c;
    float humidity_pct;
    bool sensor_valid;
    uint32_t sensor_read_count;
    uint32_t sensor_error_count;
    uint32_t last_sensor_ms;

    float error_c;
    bool relay_requested;
    bool relay_output;
    float pid_kp;
    float pid_ki;
    float pid_kd;
    float pid_p;
    float pid_i;
    float pid_d;
    float pid_integral;
    float pid_output_pct;
    float output_pct;
    uint32_t pwm_duty;

    bool plotter_enabled;
    bool command_error;
    system_status_t status;
    command_kind_t last_command_kind;
    uint32_t command_count;
    uint32_t invalid_command_count;
    uint32_t last_command_ms;
} system_state_t;

typedef struct {
    system_state_t state;
    SemaphoreHandle_t mutex;
} app_context_t;

void system_state_init(app_context_t *ctx);
bool system_state_snapshot(app_context_t *ctx, system_state_t *out);

void system_state_set_sensor(app_context_t *ctx,
                             float temperature_c,
                             float humidity_pct,
                             uint32_t timestamp_ms,
                             bool valid);
void system_state_set_control_output(app_context_t *ctx,
                                     float error_c,
                                     bool relay_requested,
                                     float output_pct,
                                     float pid_p,
                                     float pid_i,
                                     float pid_d,
                                     float pid_integral);
void system_state_set_actuator_output(app_context_t *ctx,
                                      bool relay_output,
                                      float pwm_pct,
                                      uint32_t pwm_duty);

void system_state_set_mode(app_context_t *ctx, control_mode_t mode, uint32_t timestamp_ms);
void system_state_set_setpoint(app_context_t *ctx, float setpoint_c, uint32_t timestamp_ms);
void system_state_set_hysteresis(app_context_t *ctx, float hysteresis_c, uint32_t timestamp_ms);
void system_state_set_pid(app_context_t *ctx,
                          float kp,
                          float ki,
                          float kd,
                          uint32_t timestamp_ms);
void system_state_set_plotter(app_context_t *ctx, bool enabled, uint32_t timestamp_ms);
void system_state_record_status_request(app_context_t *ctx, uint32_t timestamp_ms);
void system_state_record_invalid_command(app_context_t *ctx, uint32_t timestamp_ms);

#endif // SYSTEM_STATE_H
