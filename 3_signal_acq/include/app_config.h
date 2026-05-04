#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"

// GPIO mapping
#define PIN_DHT_DATA         GPIO_NUM_4
#define PIN_PIR_OUT          GPIO_NUM_18
#define PIN_BUTTON_RESET     GPIO_NUM_19
#define PIN_LED_MOTION       GPIO_NUM_23
#define PIN_LED_LIGHT        GPIO_NUM_25
#define PIN_LDR_ADC          GPIO_NUM_34

// I2C / OLED
#define I2C_PORT             I2C_NUM_0
#define I2C_SDA_PIN          GPIO_NUM_21
#define I2C_SCL_PIN          GPIO_NUM_22
#define I2C_FREQ_HZ          100000
#define OLED_I2C_ADDR        0x3C

// Task timing (ms)
#define DHT_SAMPLE_MS        1500
#define MOTION_POLL_MS       50
#define MOTION_STABLE_SAMPLES 2
#define LIGHT_SAMPLE_MS      50
#define BUTTON_POLL_MS       20
#define BUTTON_DEBOUNCE_MS   40
#define RESET_HOLD_MS        800
#define DISPLAY_REFRESH_MS   300
#define PROCESSING_POLL_MS   100

// Alert thresholds + hysteresis
#define TEMP_ALERT_ON_C      26.0f
#define TEMP_ALERT_OFF_C     24.0f
#define TEMP_ALERT_ON_CONFIRM_SAMPLES 2
#define HUM_ALERT_ON_PCT     70.0f
#define HUM_ALERT_OFF_PCT    65.0f
#define LIGHT_ADC_INVERTED   1
#define LIGHT_ALERT_ON_RAW   2200U
#define LIGHT_ALERT_OFF_RAW  2600U
#define LIGHT_RAW_MIN        0U
#define LIGHT_RAW_MAX        4095U
// Brightness percentage uses a narrower practical ADC window than the full 12-bit range.
// Tune these for your real LDR module if needed.
#define LIGHT_PCT_MIN_RAW    0U
#define LIGHT_PCT_MAX_RAW    2400U

// DHT sanity limits used to reject corrupted frames on real hardware.
#define DHT_TEMP_MIN_C       -40.0f
#define DHT_TEMP_MAX_C       80.0f
#define DHT_HUM_MIN_PCT      0.0f
#define DHT_HUM_MAX_PCT      100.0f

// ADC (ESP32 ADC1, GPIO34 -> channel 6)
#define LDR_ADC_UNIT         ADC_UNIT_1
#define LDR_ADC_CHANNEL      ADC_CHANNEL_6
#define LDR_ADC_ATTEN        ADC_ATTEN_DB_12
#define LDR_ADC_BITWIDTH     ADC_BITWIDTH_12

// Task configuration
#define TASK_STACK_DEFAULT   3072
// Priority rationale:
// - Motion, light, and button are highest to keep event reaction under 100 ms.
// - DHT/processing are mid-priority periodic tasks.
// - Display is lowest so UI refresh never blocks input/sensing.
#define TASK_PRIO_DHT        3
#define TASK_PRIO_MOTION     4
#define TASK_PRIO_LIGHT      4
#define TASK_PRIO_BUTTON     4
#define TASK_PRIO_PROCESS    3
#define TASK_PRIO_DISPLAY    2

#endif // APP_CONFIG_H
