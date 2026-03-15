#include "dht11_driver.h"

#include <stdbool.h>
#include <stdint.h>

#include "app_config.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"

#define DHT_START_LOW_MS         20
#define DHT_START_RELEASE_US     30
#define DHT_RESP_TIMEOUT_US      200
#define DHT_BIT_HIGH_TIMEOUT_US  220
#define DHT_BIT_ONE_THRESHOLD_US 45
#define DHT_READ_RETRIES         2
#define DHT_RETRY_DELAY_MS       20

static portMUX_TYPE s_dht_mux = portMUX_INITIALIZER_UNLOCKED;

static esp_err_t wait_for_level(int level, uint32_t timeout_us)
{
    const int64_t start = esp_timer_get_time();
    while (gpio_get_level(PIN_DHT_DATA) != level) {
        if ((esp_timer_get_time() - start) > timeout_us) {
            return ESP_ERR_TIMEOUT;
        }
    }
    return ESP_OK;
}

esp_err_t dht11_init(void)
{
    gpio_config_t cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_DHT_DATA),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    return gpio_config(&cfg);
}

esp_err_t dht11_read(float *temperature_c, float *humidity_pct)
{
    if (temperature_c == NULL || humidity_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t last_err = ESP_FAIL;

    for (int attempt = 0; attempt <= DHT_READ_RETRIES; ++attempt) {
        uint8_t data[5] = {0};

        gpio_set_direction(PIN_DHT_DATA, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(PIN_DHT_DATA, 0);
        vTaskDelay(pdMS_TO_TICKS(DHT_START_LOW_MS));

        gpio_set_level(PIN_DHT_DATA, 1);
        ets_delay_us(DHT_START_RELEASE_US);
        gpio_set_direction(PIN_DHT_DATA, GPIO_MODE_INPUT);

        // Keep the timing-critical pulse decoding non-preemptible.
        portENTER_CRITICAL(&s_dht_mux);

        last_err = wait_for_level(0, DHT_RESP_TIMEOUT_US);
        if (last_err == ESP_OK) {
            last_err = wait_for_level(1, DHT_RESP_TIMEOUT_US);
        }
        if (last_err == ESP_OK) {
            last_err = wait_for_level(0, DHT_RESP_TIMEOUT_US);
        }

        for (int i = 0; i < 40 && last_err == ESP_OK; ++i) {
            last_err = wait_for_level(1, DHT_RESP_TIMEOUT_US);
            if (last_err != ESP_OK) {
                break;
            }

            const int64_t high_start = esp_timer_get_time();
            last_err = wait_for_level(0, DHT_BIT_HIGH_TIMEOUT_US);
            if (last_err != ESP_OK) {
                break;
            }
            const int64_t high_us = esp_timer_get_time() - high_start;

            data[i / 8] <<= 1;
            if (high_us > DHT_BIT_ONE_THRESHOLD_US) {
                data[i / 8] |= 1;
            }
        }

        portEXIT_CRITICAL(&s_dht_mux);

        if (last_err != ESP_OK) {
            if (attempt < DHT_READ_RETRIES) {
                vTaskDelay(pdMS_TO_TICKS(DHT_RETRY_DELAY_MS));
            }
            continue;
        }

        const uint8_t checksum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
        if (checksum != data[4]) {
            last_err = ESP_ERR_INVALID_CRC;
            if (attempt < DHT_READ_RETRIES) {
                vTaskDelay(pdMS_TO_TICKS(DHT_RETRY_DELAY_MS));
            }
            continue;
        }

        // DHT11: integer bytes in data[0]/data[2], decimals usually 0.
        // DHT22 compatibility for simulation: packed 16-bit values with 0.1 resolution.
        const bool looks_like_dht22 = (data[1] != 0U) || (data[3] != 0U);

        if (looks_like_dht22) {
            const uint16_t raw_h = (uint16_t)((data[0] << 8) | data[1]);
            uint16_t raw_t = (uint16_t)(((data[2] & 0x7F) << 8) | data[3]);
            float temp = (float)raw_t / 10.0f;
            if (data[2] & 0x80) {
                temp = -temp;
            }

            *humidity_pct = (float)raw_h / 10.0f;
            *temperature_c = temp;
        } else {
            *humidity_pct = (float)data[0];
            *temperature_c = (float)data[2];
        }

        return ESP_OK;
    }

    return last_err;
}
