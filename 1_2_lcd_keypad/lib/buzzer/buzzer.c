#include "buzzer.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";

esp_err_t buzzer_init(void)
{
    esp_err_t ret;
    
    // Configure buzzer pin as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure buzzer pin: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Turn buzzer off initially
    buzzer_off();
    
    ESP_LOGI(TAG, "Buzzer initialized successfully");
    return ESP_OK;
}

void buzzer_on(void)
{
    gpio_set_level(GPIO_BUZZER, 1);
}

void buzzer_off(void)
{
    gpio_set_level(GPIO_BUZZER, 0);
}

void buzzer_beep(uint32_t duration_ms)
{
    buzzer_on();
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    buzzer_off();
    ESP_LOGD(TAG, "Beeped for %lu ms", duration_ms);
}

void buzzer_success(void)
{
    buzzer_beep(BUZZER_SUCCESS_DURATION_MS);
}

void buzzer_error(void)
{
    // Triple beep pattern: beep-pause-beep-pause-beep
    for (int i = 0; i < BUZZER_ERROR_REPEAT; i++) {
        buzzer_beep(BUZZER_ERROR_BEEP_MS);
        if (i < BUZZER_ERROR_REPEAT - 1) {
            vTaskDelay(pdMS_TO_TICKS(BUZZER_ERROR_PAUSE_MS));
        }
    }
    ESP_LOGD(TAG, "Error pattern played");
}
