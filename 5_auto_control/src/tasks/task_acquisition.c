#include "task_acquisition.h"

#include "app_config.h"
#include "app_utils.h"
#include "dht/dht_sensor.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "task_acquisition";

static void task_acquisition_fn(void *arg)
{
    app_context_t *ctx = (app_context_t *)arg;
    vTaskDelay(pdMS_TO_TICKS(1000));

    while (true) {
        float temperature_c = 0.0f;
        float humidity_pct = 0.0f;
        const esp_err_t err = dht_sensor_read(&temperature_c, &humidity_pct);

        system_state_set_sensor(ctx, temperature_c, humidity_pct, app_now_ms(), err == ESP_OK);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "DHT read failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(DHT_SAMPLE_MS));
    }
}

bool task_acquisition_start(app_context_t *ctx)
{
    const BaseType_t rc = xTaskCreate(task_acquisition_fn,
                                      "TaskAcquire",
                                      TASK_STACK_DEFAULT,
                                      ctx,
                                      TASK_PRIO_ACQUISITION,
                                      NULL);
    if (rc != pdPASS) {
        ESP_LOGE(TAG, "failed to create TaskAcquire");
        return false;
    }
    return true;
}
