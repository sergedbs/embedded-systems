#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "driver/i2c.h"

// GPIO mapping
#define PIN_DHT_DATA         GPIO_NUM_4
#define PIN_PIR_OUT          GPIO_NUM_18
#define PIN_BUTTON_RESET     GPIO_NUM_19
#define PIN_LED_MOTION       GPIO_NUM_23

// I2C / OLED
#define I2C_PORT             I2C_NUM_0
#define I2C_SDA_PIN          GPIO_NUM_21
#define I2C_SCL_PIN          GPIO_NUM_22
#define I2C_FREQ_HZ          100000
#define OLED_I2C_ADDR        0x3C

// Task timing (ms)
#define DHT_SAMPLE_MS        1500
#define MOTION_POLL_MS       50
#define BUTTON_POLL_MS       20
#define BUTTON_DEBOUNCE_MS   40
#define DISPLAY_REFRESH_MS   300
#define PROCESSING_POLL_MS   100

// Alert thresholds + hysteresis
#define TEMP_ALERT_ON_C      26.0f
#define TEMP_ALERT_OFF_C     24.0f
#define HUM_ALERT_ON_PCT     70.0f
#define HUM_ALERT_OFF_PCT    65.0f

// Task configuration
#define TASK_STACK_DEFAULT   3072
#define TASK_PRIO_DHT        3
#define TASK_PRIO_MOTION     4
#define TASK_PRIO_BUTTON     4
#define TASK_PRIO_PROCESS    3
#define TASK_PRIO_DISPLAY    2

#endif // APP_CONFIG_H
