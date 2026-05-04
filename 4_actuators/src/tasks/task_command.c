#include "task_command.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_config.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_command";

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

static const char *const BIN_ON_COMMANDS[] = {
    "BIN ON",
    "BINON",
    "ON",
    "RELAY ON",
    "RELAYON",
};

static const char *const BIN_OFF_COMMANDS[] = {
    "BIN OFF",
    "BINOFF",
    "OFF",
    "RELAY OFF",
    "RELAYOFF",
};

static const char *const IMMEDIATE_COMMANDS[] = {
    "BIN ON",
    "BINON",
    "ON",
    "BIN OFF",
    "BINOFF",
    "OFF",
    "RELAY ON",
    "RELAYON",
    "RELAY OFF",
    "RELAYOFF",
    "STOP",
    "STATUS",
    "ALERT",
};

static bool command_matches(const char *line, const char *const *commands, size_t count)
{
    if (line == NULL || commands == NULL) {
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        if (strcmp(line, commands[i]) == 0) {
            return true;
        }
    }
    return false;
}

static void trim_line(char *line)
{
    if (line == NULL) {
        return;
    }

    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || isspace((unsigned char)line[len - 1]))) {
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

static bool parse_motor_value(const char *text, int *out)
{
    if (text == NULL || out == NULL) {
        return false;
    }

    char *end = NULL;
    const long value = strtol(text, &end, 10);
    while (end != NULL && *end != '\0' && isspace((unsigned char)*end)) {
        end++;
    }
    if (end == text || (end != NULL && *end != '\0')) {
        return false;
    }
    *out = (int)value;
    return true;
}

static bool is_complete_known_command(const char *line)
{
    return command_matches(line, IMMEDIATE_COMMANDS,
                           sizeof(IMMEDIATE_COMMANDS) / sizeof(IMMEDIATE_COMMANDS[0]));
}

static void process_command(app_context_t *ctx, char *line)
{
    trim_line(line);
    if (line == NULL || line[0] == '\0') {
        return;
    }

    uppercase(line);
    const uint32_t ts = now_ms();

    if (command_matches(line, BIN_ON_COMMANDS, sizeof(BIN_ON_COMMANDS) / sizeof(BIN_ON_COMMANDS[0]))) {
        system_state_set_binary_command(ctx, true, ts);
        ESP_LOGI(TAG, "accepted command: binary actuator ON");
        return;
    }

    if (command_matches(line, BIN_OFF_COMMANDS, sizeof(BIN_OFF_COMMANDS) / sizeof(BIN_OFF_COMMANDS[0]))) {
        system_state_set_binary_command(ctx, false, ts);
        ESP_LOGI(TAG, "accepted command: binary actuator OFF");
        return;
    }

    if (strcmp(line, "STOP") == 0 || strcmp(line, "MOTOR 0") == 0) {
        system_state_set_motor_command(ctx, 0, ts, false);
        ESP_LOGI(TAG, "accepted command: motor soft-stop");
        return;
    }

    if (strcmp(line, "STATUS") == 0) {
        system_state_record_status_request(ctx, ts);
        ESP_LOGI(TAG, "accepted command: status request");
        return;
    }

    if (strcmp(line, "ALERT") == 0) {
        system_state_request_alert_page(ctx, ts, LCD_ALERT_PAGE_MS);
        ESP_LOGI(TAG, "accepted command: LCD alert page");
        return;
    }

    if (strncmp(line, "MOTOR ", 6) == 0 || strncmp(line, "MOTOR", 5) == 0 || line[0] == 'M') {
        int target = 0;
        const char *value_text = NULL;
        if (strncmp(line, "MOTOR ", 6) == 0) {
            value_text = line + 6;
        } else if (strncmp(line, "MOTOR", 5) == 0) {
            value_text = line + 5;
        } else {
            value_text = line + 1;
        }

        while (*value_text != '\0' && isspace((unsigned char)*value_text)) {
            value_text++;
        }

        if (parse_motor_value(value_text, &target)) {
            const bool limited = (target < MOTOR_TARGET_MIN_PCT || target > MOTOR_TARGET_MAX_PCT);
            system_state_set_motor_command(ctx, target, ts, limited);
            ESP_LOGI(TAG, "accepted command: motor target=%d%% limited=%d", target, limited);
            return;
        }
    }

    system_state_record_invalid_command(ctx, ts);
    ESP_LOGW(TAG, "invalid command: '%s'", line);
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
    char line[64] = {0};
    size_t len = 0;
    uint32_t last_rx_ms = 0;

    const int flags = fcntl(fileno(stdin), F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(fileno(stdin), F_SETFL, flags | O_NONBLOCK);
    }

    ESP_LOGI(TAG, "serial commands: BIN ON | BIN OFF | MOTOR -100..100 | STOP | STATUS | ALERT");
    while (true) {
        const int ch = getchar();
        const uint32_t now = now_ms();

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
                line[len++] = (char)toupper((unsigned char)ch);
                line[len] = '\0';
                if (is_complete_known_command(line)) {
                    process_command(ctx, line);
                    clear_line(line, &len);
                }
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
