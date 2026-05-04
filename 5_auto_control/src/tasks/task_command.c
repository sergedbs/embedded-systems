#include "task_command.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_config.h"
#include "app_utils.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_command";

static void trim_line(char *line)
{
    if (line == NULL) {
        return;
    }

    size_t len = strlen(line);
    while (len > 0U && isspace((unsigned char)line[len - 1U])) {
        line[--len] = '\0';
    }

    char *start = line;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != line) {
        memmove(line, start, strlen(start) + 1U);
    }
}

static void uppercase(char *line)
{
    if (line == NULL) {
        return;
    }
    for (; *line != '\0'; ++line) {
        *line = (char)toupper((unsigned char)*line);
    }
}

static bool parse_float(const char *text, float *out)
{
    if (text == NULL || out == NULL) {
        return false;
    }

    while (*text != '\0' && isspace((unsigned char)*text)) {
        text++;
    }

    char *end = NULL;
    const float value = strtof(text, &end);
    while (end != NULL && *end != '\0' && isspace((unsigned char)*end)) {
        end++;
    }

    if (end == text || (end != NULL && *end != '\0') || !app_float_is_valid(value)) {
        return false;
    }

    *out = value;
    return true;
}

static bool parse_three_floats(const char *text, float *a, float *b, float *c)
{
    if (text == NULL || a == NULL || b == NULL || c == NULL) {
        return false;
    }

    char *end1 = NULL;
    char *end2 = NULL;
    char *end3 = NULL;
    *a = strtof(text, &end1);
    *b = strtof(end1, &end2);
    *c = strtof(end2, &end3);

    while (end3 != NULL && *end3 != '\0' && isspace((unsigned char)*end3)) {
        end3++;
    }

    return end1 != text &&
           end2 != end1 &&
           end3 != end2 &&
           end3 != NULL &&
           *end3 == '\0' &&
           app_float_is_valid(*a) &&
           app_float_is_valid(*b) &&
           app_float_is_valid(*c);
}

static void print_status(const system_state_t *s)
{
    if (s == NULL) {
        return;
    }

    ESP_LOGI(TAG,
             "mode=%s sp=%.2f temp=%.2f filt=%.2f hum=%.1f err=%.2f relay=%d out=%.1f duty=%lu kp=%.2f ki=%.2f kd=%.2f cmds=%lu bad=%lu",
             s->mode == CONTROL_MODE_PID ? "PID" : "ONOFF",
             s->setpoint_c,
             s->temperature_c,
             s->filtered_temperature_c,
             s->humidity_pct,
             s->error_c,
             s->relay_output,
             s->output_pct,
             (unsigned long)s->pwm_duty,
             s->pid_kp,
             s->pid_ki,
             s->pid_kd,
             (unsigned long)s->command_count,
             (unsigned long)s->invalid_command_count);
}

static void print_help(void)
{
    ESP_LOGI(TAG, "commands: MODE ONOFF|PID, SP <C>, HYST <C>, PID <kp> <ki> <kd>, KP <v>, KI <v>, KD <v>, PLOT ON|OFF, STATUS");
}

static void process_command(app_context_t *ctx, char *line)
{
    trim_line(line);
    if (line == NULL || line[0] == '\0') {
        return;
    }

    uppercase(line);
    const uint32_t ts = app_now_ms();

    if (strcmp(line, "HELP") == 0 || strcmp(line, "?") == 0) {
        print_help();
        system_state_record_status_request(ctx, ts);
        return;
    }

    if (strcmp(line, "STATUS") == 0) {
        system_state_t s = {0};
        system_state_record_status_request(ctx, ts);
        if (system_state_snapshot(ctx, &s)) {
            print_status(&s);
        }
        return;
    }

    if (strcmp(line, "ONOFF") == 0 || strcmp(line, "MODE ONOFF") == 0 || strcmp(line, "MODE ON-OFF") == 0) {
        system_state_set_mode(ctx, CONTROL_MODE_ON_OFF, ts);
        ESP_LOGI(TAG, "accepted command: mode ON-OFF");
        return;
    }

    if (strcmp(line, "PID") == 0 || strcmp(line, "MODE PID") == 0) {
        system_state_set_mode(ctx, CONTROL_MODE_PID, ts);
        ESP_LOGI(TAG, "accepted command: mode PID");
        return;
    }

    if (strncmp(line, "SP ", 3) == 0 || strncmp(line, "SET ", 4) == 0 || strncmp(line, "SETPOINT ", 9) == 0) {
        const char *value_text = strchr(line, ' ');
        float value = 0.0f;
        if (parse_float(value_text, &value)) {
            system_state_set_setpoint(ctx, value, ts);
            ESP_LOGI(TAG, "accepted command: setpoint %.2f C", value);
            return;
        }
    }

    if (strncmp(line, "HYST ", 5) == 0 || strncmp(line, "H ", 2) == 0) {
        const char *value_text = strchr(line, ' ');
        float value = 0.0f;
        if (parse_float(value_text, &value)) {
            system_state_set_hysteresis(ctx, value, ts);
            ESP_LOGI(TAG, "accepted command: hysteresis %.2f C", value);
            return;
        }
    }

    if (strncmp(line, "PID ", 4) == 0) {
        float kp = 0.0f;
        float ki = 0.0f;
        float kd = 0.0f;
        if (parse_three_floats(line + 4, &kp, &ki, &kd)) {
            system_state_set_pid(ctx, kp, ki, kd, ts);
            ESP_LOGI(TAG, "accepted command: PID %.2f %.2f %.2f", kp, ki, kd);
            return;
        }
    }

    if (strncmp(line, "KP ", 3) == 0 || strncmp(line, "KI ", 3) == 0 || strncmp(line, "KD ", 3) == 0) {
        float value = 0.0f;
        if (parse_float(line + 3, &value)) {
            system_state_t s = {0};
            if (system_state_snapshot(ctx, &s)) {
                float kp = s.pid_kp;
                float ki = s.pid_ki;
                float kd = s.pid_kd;
                if (line[1] == 'P') {
                    kp = value;
                } else if (line[1] == 'I') {
                    ki = value;
                } else {
                    kd = value;
                }
                system_state_set_pid(ctx, kp, ki, kd, ts);
                ESP_LOGI(TAG, "accepted command: PID %.2f %.2f %.2f", kp, ki, kd);
                return;
            }
        }
    }

    if (strcmp(line, "PLOT ON") == 0 || strcmp(line, "PLOTON") == 0) {
        system_state_set_plotter(ctx, true, ts);
        ESP_LOGI(TAG, "accepted command: plotter ON");
        return;
    }

    if (strcmp(line, "PLOT OFF") == 0 || strcmp(line, "PLOTOFF") == 0) {
        system_state_set_plotter(ctx, false, ts);
        ESP_LOGI(TAG, "accepted command: plotter OFF");
        return;
    }

    system_state_record_invalid_command(ctx, ts);
    ESP_LOGW(TAG, "invalid command: '%s'", line);
    print_help();
}

static void clear_line(char *line, size_t *len)
{
    if (line != NULL) {
        line[0] = '\0';
    }
    if (len != NULL) {
        *len = 0;
    }
}

static void task_command_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    char line[96] = {0};
    size_t len = 0;
    uint32_t last_rx_ms = 0;

    const int flags = fcntl(fileno(stdin), F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);
    }

    print_help();
    while (true) {
        const int ch = getchar();
        const uint32_t now = app_now_ms();

        if (ch != EOF) {
            last_rx_ms = now;

            if (ch == '\n' || ch == '\r') {
                if (len > 0U) {
                    line[len] = '\0';
                    process_command(ctx, line);
                    clear_line(line, &len);
                }
                continue;
            }

            if (ch == '\b' || ch == 0x7F) {
                if (len > 0U) {
                    line[--len] = '\0';
                }
                continue;
            }

            if (isprint((unsigned char)ch) && len < (sizeof(line) - 1U)) {
                line[len++] = (char)ch;
                line[len] = '\0';
            }
        } else {
            clearerr(stdin);
        }

        if (len > 0U && last_rx_ms > 0U && (now - last_rx_ms) >= COMMAND_IDLE_COMMIT_MS) {
            line[len] = '\0';
            process_command(ctx, line);
            clear_line(line, &len);
        }

        vTaskDelay(pdMS_TO_TICKS(COMMAND_IDLE_DELAY_MS));
    }
}

bool task_command_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_command_fn, "TaskCommand", TASK_STACK_COMMAND, ctx, TASK_PRIO_COMMAND, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskCommand");
        return false;
    }
    return true;
}
