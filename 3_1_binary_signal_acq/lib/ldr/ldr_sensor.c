#include "ldr_sensor.h"

#include "app_config.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

esp_err_t ldr_sensor_init(void)
{
    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = LDR_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) {
        return err;
    }

    const adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = LDR_ADC_ATTEN,
        .bitwidth = LDR_ADC_BITWIDTH,
    };

    return adc_oneshot_config_channel(s_adc_handle, LDR_ADC_CHANNEL, &ch_cfg);
}

esp_err_t ldr_sensor_read_raw(uint16_t *value)
{
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_adc_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    int raw = 0;
    const esp_err_t err = adc_oneshot_read(s_adc_handle, LDR_ADC_CHANNEL, &raw);
    if (err != ESP_OK) {
        return err;
    }

    if (raw < 0) {
        raw = 0;
    }
    if (raw > 4095) {
        raw = 4095;
    }

    *value = (uint16_t)raw;
    return ESP_OK;
}
