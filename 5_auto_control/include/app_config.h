#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"

// GPIO mapping
#define PIN_DHT_DATA            GPIO_NUM_18
#define PIN_RELAY_HEATER        GPIO_NUM_23
#define PIN_PWM_HEATER          GPIO_NUM_25

// Hardware profile switches. These can be overridden from platformio.ini build_flags.
// Wokwi relay modules and LEDs are configured for active-high commands.
#ifndef RELAY_HEATER_ACTIVE_LOW
#define RELAY_HEATER_ACTIVE_LOW 0
#endif

#ifndef PWM_HEATER_ACTIVE_LOW
#define PWM_HEATER_ACTIVE_LOW 0
#endif

// I2C / LCD1602 via PCF8574 backpack
#define I2C_PORT                I2C_NUM_0
#define I2C_SDA_PIN             GPIO_NUM_21
#define I2C_SCL_PIN             GPIO_NUM_22
#define I2C_FREQ_HZ             100000
#define LCD_I2C_ADDR_1          0x27
#define LCD_I2C_ADDR_2          0x3F
#define LCD_COLS                16
#define LCD_ROWS                2

// Task timing (ms)
#define COMMAND_IDLE_DELAY_MS   20
#define COMMAND_IDLE_COMMIT_MS  1200
#define COMMAND_ERROR_HOLD_MS   3000
#define DHT_SAMPLE_MS           2000
#define CONTROL_PERIOD_MS       200
#define ACTUATOR_OUTPUT_MS      100
#define DISPLAY_REFRESH_MS      500
#define PLOTTER_PERIOD_MS       500

// DHT sanity limits used to reject corrupted frames on real hardware.
#define DHT_TEMP_MIN_C          -40.0f
#define DHT_TEMP_MAX_C          80.0f
#define DHT_HUM_MIN_PCT         0.0f
#define DHT_HUM_MAX_PCT         100.0f

// Temperature-control defaults
#define CONTROL_SETPOINT_C      25.0f
#define CONTROL_HYSTERESIS_C    1.0f
#define CONTROL_OUTPUT_MIN_PCT  0.0f
#define CONTROL_OUTPUT_MAX_PCT  100.0f
#define PID_KP_DEFAULT          18.0f
#define PID_KI_DEFAULT          0.25f
#define PID_KD_DEFAULT          6.0f
#define PID_INTEGRAL_MIN        -100.0f
#define PID_INTEGRAL_MAX        100.0f

// PWM / LEDC configuration
#define HEATER_LEDC_MODE        LEDC_HIGH_SPEED_MODE
#define HEATER_LEDC_TIMER       LEDC_TIMER_0
#define HEATER_LEDC_CHANNEL     LEDC_CHANNEL_0
#define HEATER_PWM_FREQ_HZ      5000
#define HEATER_PWM_RESOLUTION   LEDC_TIMER_10_BIT
#define HEATER_PWM_MAX_DUTY     1023U

// Task configuration
#define TASK_STACK_DEFAULT      3072
#define TASK_STACK_COMMAND      4096
#define TASK_PRIO_COMMAND       4
#define TASK_PRIO_ACQUISITION   3
#define TASK_PRIO_CONTROL       4
#define TASK_PRIO_OUTPUT        4
#define TASK_PRIO_DISPLAY       2

#endif // APP_CONFIG_H
