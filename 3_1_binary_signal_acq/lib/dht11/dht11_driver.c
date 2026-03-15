#include "dht11_driver.h"

#include "esp_err.h"

esp_err_t dht11_init(void)
{
    return ESP_OK;
}

esp_err_t dht11_read(float *temperature_c, float *humidity_pct)
{
    if (temperature_c == NULL || humidity_pct == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Placeholder values for iteration 2 structure bring-up.
    *temperature_c = 24.0f;
    *humidity_pct = 52.0f;
    return ESP_OK;
}
