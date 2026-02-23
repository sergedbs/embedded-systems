#include "buzzer.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";

static void buzzer_on(void)
{
    gpio_set_level(GPIO_BUZZER, 1);
}

static void buzzer_off(void)
{
    gpio_set_level(GPIO_BUZZER, 0);
}

esp_err_t buzzer_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure buzzer pin: %s", esp_err_to_name(ret));
        return ret;
    }

    buzzer_off();
    ESP_LOGI(TAG, "Buzzer initialized");
    return ESP_OK;
}

void buzzer_beep(uint32_t duration_ms)
{
    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_off();
}

void buzzer_success(void)
{
    buzzer_beep(BUZZER_SUCCESS_DURATION_MS);
}

void buzzer_error(void)
{
    for (int i = 0; i < BUZZER_ERROR_REPEAT; i++) {
        buzzer_beep(BUZZER_ERROR_BEEP_MS);
        if (i < BUZZER_ERROR_REPEAT - 1) {
            vTaskDelay(pdMS_TO_TICKS(BUZZER_ERROR_PAUSE_MS));
        }
    }
}
