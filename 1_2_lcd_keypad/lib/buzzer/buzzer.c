#include "buzzer.h"
#include "config_pins.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "BUZZER";

// Duty value for "on" state: scale BUZZER_VOLUME_PERCENT (0-100) to 8-bit range
#define BUZZER_DUTY_ON  ((BUZZER_VOLUME_PERCENT * 255) / 100)

static void buzzer_on(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY_ON);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

static void buzzer_off(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

esp_err_t buzzer_init(void)
{
    // Configure LEDC timer
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .timer_num       = BUZZER_LEDC_TIMER,
        .duty_resolution = BUZZER_LEDC_RESOLUTION,
        .freq_hz         = BUZZER_LEDC_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure LEDC channel
    ledc_channel_config_t ch_conf = {
        .gpio_num   = GPIO_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BUZZER_LEDC_CHANNEL,
        .timer_sel  = BUZZER_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ret = ledc_channel_config(&ch_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_channel_config failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Buzzer initialized (volume %d%%, duty %d/255)",
             BUZZER_VOLUME_PERCENT, BUZZER_DUTY_ON);
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
