#include "task_light.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "filters/filters.h"
#include "freertos/task.h"
#include "ldr/ldr_sensor.h"

static const char *TAG = "task_light";

static bool light_threshold_on(uint16_t value)
{
#if LIGHT_ADC_INVERTED
    return value <= LIGHT_ALERT_ON_RAW;
#else
    return value >= LIGHT_ALERT_ON_RAW;
#endif
}

static bool light_threshold_off(uint16_t value)
{
#if LIGHT_ADC_INVERTED
    return value >= LIGHT_ALERT_OFF_RAW;
#else
    return value <= LIGHT_ALERT_OFF_RAW;
#endif
}

static void task_light_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;

    filter_median3_t f_med;
    filter_sma3_t f_sma;
    filter_wma4_t f_wma;
    filter_median3_reset(&f_med);
    filter_sma3_reset(&f_sma);
    filter_wma4_reset(&f_wma);

    bool light_alert = false;
    uint32_t last_reset_count = 0;

    while (true) {
        system_state_t snapshot = {0};
        if (system_state_snapshot(ctx, &snapshot) && snapshot.reset_count != last_reset_count) {
            filter_median3_reset(&f_med);
            filter_sma3_reset(&f_sma);
            filter_wma4_reset(&f_wma);
            light_alert = false;
            gpio_set_level(PIN_LED_LIGHT, 0);
            system_state_set_light_alert(ctx, false, false);
            last_reset_count = snapshot.reset_count;
        }

        if (system_state_snapshot(ctx, &snapshot)) {
            const uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            if (now < snapshot.reset_hold_until_ms) {
                filter_median3_reset(&f_med);
                filter_sma3_reset(&f_sma);
                filter_wma4_reset(&f_wma);
                light_alert = false;
                gpio_set_level(PIN_LED_LIGHT, 0);
                system_state_set_light_alert(ctx, false, false);
                system_state_set_light(ctx, 0, 0, 0, 0, 0, now, true);
                vTaskDelay(pdMS_TO_TICKS(LIGHT_SAMPLE_MS));
                continue;
            }
        }

        uint16_t raw = 0;
        const esp_err_t err = ldr_sensor_read_raw(&raw);

        const uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        if (err != ESP_OK) {
            system_state_set_light(ctx, 0, 0, 0, 0, 0, now, false);
            ESP_LOGW(TAG, "ldr read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(LIGHT_SAMPLE_MS));
            continue;
        }

        const uint16_t clamped = filter_clamp_u16(raw, LIGHT_RAW_MIN, LIGHT_RAW_MAX);
        const uint16_t median = filter_median3_apply(&f_med, clamped);
        const uint16_t avg = filter_sma3_apply(&f_sma, median);
        const uint16_t weighted = filter_wma4_apply(&f_wma, avg);

        if (!light_alert && light_threshold_on(weighted)) {
            light_alert = true;
        } else if (light_alert && light_threshold_off(weighted)) {
            light_alert = false;
        }

        gpio_set_level(PIN_LED_LIGHT, light_alert ? 1 : 0);

        system_state_set_light(ctx, raw, clamped, median, avg, weighted, now, true);
        system_state_set_light_alert(ctx, light_alert, light_alert);

        vTaskDelay(pdMS_TO_TICKS(LIGHT_SAMPLE_MS));
    }
}

bool task_light_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_light_fn, "TaskLightSensor", TASK_STACK_DEFAULT, ctx, TASK_PRIO_LIGHT, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskLightSensor");
        return false;
    }
    return true;
}
