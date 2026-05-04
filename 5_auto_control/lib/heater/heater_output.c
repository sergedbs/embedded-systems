#include "heater_output.h"

#include "app_config.h"
#include "app_utils.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_check.h"

static const char *TAG = "heater_output";

static bool s_relay_on;
static float s_pwm_pct;

static int relay_level(bool on)
{
#if RELAY_HEATER_ACTIVE_LOW
    return on ? 0 : 1;
#else
    return on ? 1 : 0;
#endif
}

uint32_t heater_output_pct_to_duty(float pct)
{
    const float clamped = app_clamp_float(pct, CONTROL_OUTPUT_MIN_PCT, CONTROL_OUTPUT_MAX_PCT);
    return (uint32_t)((clamped * (float)HEATER_PWM_MAX_DUTY) / 100.0f);
}

esp_err_t heater_output_init(void)
{
    const gpio_config_t relay_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_RELAY_HEATER),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&relay_cfg), TAG, "relay gpio config failed");

    const ledc_timer_config_t timer_cfg = {
        .speed_mode = HEATER_LEDC_MODE,
        .timer_num = HEATER_LEDC_TIMER,
        .duty_resolution = HEATER_PWM_RESOLUTION,
        .freq_hz = HEATER_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "pwm timer config failed");

    const ledc_channel_config_t channel_cfg = {
        .gpio_num = PIN_PWM_HEATER,
        .speed_mode = HEATER_LEDC_MODE,
        .channel = HEATER_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = HEATER_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_cfg), TAG, "pwm channel config failed");

    ESP_RETURN_ON_ERROR(heater_output_set_relay(false), TAG, "relay off failed");
    return heater_output_set_pwm_pct(0.0f, NULL);
}

esp_err_t heater_output_set_relay(bool on)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_RELAY_HEATER, relay_level(on)), TAG, "relay set failed");
    s_relay_on = on;
    return ESP_OK;
}

esp_err_t heater_output_set_pwm_pct(float pct, uint32_t *duty_out)
{
    const float clamped = app_clamp_float(pct, CONTROL_OUTPUT_MIN_PCT, CONTROL_OUTPUT_MAX_PCT);
    uint32_t duty = heater_output_pct_to_duty(clamped);

#if PWM_HEATER_ACTIVE_LOW
    duty = HEATER_PWM_MAX_DUTY - duty;
#endif

    ESP_RETURN_ON_ERROR(ledc_set_duty(HEATER_LEDC_MODE, HEATER_LEDC_CHANNEL, duty), TAG, "set duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(HEATER_LEDC_MODE, HEATER_LEDC_CHANNEL), TAG, "update duty failed");

    if (duty_out != NULL) {
        *duty_out = duty;
    }
    s_pwm_pct = clamped;
    return ESP_OK;
}

bool heater_output_get_relay(void)
{
    return s_relay_on;
}

float heater_output_get_pwm_pct(void)
{
    return s_pwm_pct;
}
