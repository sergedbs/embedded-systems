#include "analog_motor.h"

#include "app_config.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_check.h"

static const char *TAG = "analog_motor";
static int s_speed_pct;

static int motor_direction_level(bool active)
{
#if MOTOR_DIRECTION_ACTIVE_LOW
    return active ? 0 : 1;
#else
    return active ? 1 : 0;
#endif
}

uint32_t analog_motor_pct_to_duty(uint16_t pct)
{
    if (pct > MOTOR_TARGET_MAX_PCT) {
        pct = MOTOR_TARGET_MAX_PCT;
    }
    return ((uint32_t)pct * MOTOR_PWM_MAX_DUTY) / 100U;
}

static int motor_clamp_pct(int pct)
{
    if (pct > MOTOR_TARGET_MAX_PCT) {
        return MOTOR_TARGET_MAX_PCT;
    }
    if (pct < MOTOR_TARGET_MIN_PCT) {
        return MOTOR_TARGET_MIN_PCT;
    }
    return pct;
}

static uint16_t motor_magnitude_pct(int pct)
{
    pct = motor_clamp_pct(pct);
    return (uint16_t)((pct < 0) ? -pct : pct);
}

esp_err_t analog_motor_init(void)
{
#if MOTOR_DRIVER_USES_DIRECTION_PINS
    const gpio_config_t dir_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_MOTOR_IN1) | (1ULL << PIN_MOTOR_IN2),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&dir_cfg), TAG, "direction gpio config failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_MOTOR_IN1, motor_direction_level(false)), TAG, "in1 off failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_MOTOR_IN2, motor_direction_level(false)), TAG, "in2 off failed");
#endif

    const ledc_timer_config_t timer_cfg = {
        .speed_mode = MOTOR_LEDC_MODE,
        .timer_num = MOTOR_LEDC_TIMER,
        .duty_resolution = MOTOR_PWM_RESOLUTION,
        .freq_hz = MOTOR_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "ledc timer failed");

    const ledc_channel_config_t ch_cfg = {
        .gpio_num = PIN_MOTOR_PWM,
        .speed_mode = MOTOR_LEDC_MODE,
        .channel = MOTOR_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = MOTOR_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&ch_cfg), TAG, "ledc channel failed");

    return analog_motor_set_speed_pct(0, NULL);
}

esp_err_t analog_motor_set_speed_pct(int pct, uint32_t *duty_out)
{
    pct = motor_clamp_pct(pct);
    const uint16_t magnitude_pct = motor_magnitude_pct(pct);
    const bool forward = (pct > 0);
    const bool reverse = (pct < 0);

    const uint32_t duty = analog_motor_pct_to_duty(magnitude_pct);
#if MOTOR_DRIVER_USES_DIRECTION_PINS
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_MOTOR_IN1, motor_direction_level(forward)), TAG, "in1 set failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_MOTOR_IN2, motor_direction_level(reverse)), TAG, "in2 set failed");
#endif
    ESP_RETURN_ON_ERROR(ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL, duty), TAG, "set duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL), TAG, "update duty failed");

    if (duty_out != NULL) {
        *duty_out = duty;
    }
    s_speed_pct = pct;
    return ESP_OK;
}

int analog_motor_get_speed_pct(void)
{
    return s_speed_pct;
}
