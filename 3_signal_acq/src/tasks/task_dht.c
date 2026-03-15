#include "task_dht.h"

#include "app_config.h"
#include "dht11/dht11_driver.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_dht";

static void task_dht_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (true) {
        system_state_t snapshot = {0};
        if (system_state_snapshot(ctx, &snapshot)) {
            const uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
            if (now < snapshot.reset_hold_until_ms) {
                vTaskDelay(pdMS_TO_TICKS(DHT_SAMPLE_MS));
                continue;
            }
        }

        float temp = 0.0f;
        float hum = 0.0f;
        const esp_err_t err = dht11_read(&temp, &hum);

        const uint32_t now = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
        system_state_set_dht(ctx, temp, hum, now, err == ESP_OK);

        if (err != ESP_OK) {
            ESP_LOGW(TAG, "dht read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(DHT_SAMPLE_MS));
    }
}

bool task_dht_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_dht_fn, "TaskDHT", TASK_STACK_DEFAULT, ctx, TASK_PRIO_DHT, NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskDHT");
        return false;
    }
    return true;
}
