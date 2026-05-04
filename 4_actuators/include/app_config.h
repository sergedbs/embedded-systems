#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"

// GPIO mapping
#define PIN_BINARY_ACTUATOR     GPIO_NUM_23
#define PIN_MOTOR_PWM           GPIO_NUM_25
#define PIN_MOTOR_IN1           GPIO_NUM_26
#define PIN_MOTOR_IN2           GPIO_NUM_27

// Hardware profile switches. These can be overridden from platformio.ini build_flags.
// Wokwi LEDs are active-high. Many real relay modules are active-low.
#ifndef BINARY_ACTUATOR_ACTIVE_LOW
#define BINARY_ACTUATOR_ACTIVE_LOW 0
#endif

// Keep this enabled for an L298/TB6612-style driver with PWM enable + direction pins.
// The Wokwi diagram visualizes the same signals with LEDs.
#ifndef MOTOR_DRIVER_USES_DIRECTION_PINS
#define MOTOR_DRIVER_USES_DIRECTION_PINS 1
#endif

// Most L298 modules expect active-high IN1/IN2 and active-high PWM on ENA.
#ifndef MOTOR_DIRECTION_ACTIVE_LOW
#define MOTOR_DIRECTION_ACTIVE_LOW 0
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
#define COMMAND_IDLE_COMMIT_MS  2000
#define COMMAND_ERROR_HOLD_MS   3000
#define LCD_ALERT_PAGE_MS       3000
#define BINARY_CONTROL_MS       50
#define ANALOG_CONTROL_MS       50
#define ACTUATOR_OUTPUT_MS      50
#define DISPLAY_REFRESH_MS      500

// Binary actuator conditioning
#define BINARY_DEBOUNCE_MS      80

// Analog actuator conditioning. Negative values mean reverse direction.
#define MOTOR_TARGET_MIN_PCT    -100
#define MOTOR_TARGET_MAX_PCT    100
#define MOTOR_RAMP_STEP_PCT     5
#define MOTOR_OVERLOAD_PCT      95

// PWM / LEDC configuration
#define MOTOR_LEDC_MODE         LEDC_HIGH_SPEED_MODE
#define MOTOR_LEDC_TIMER        LEDC_TIMER_0
#define MOTOR_LEDC_CHANNEL      LEDC_CHANNEL_0
#define MOTOR_PWM_FREQ_HZ       5000
#define MOTOR_PWM_RESOLUTION    LEDC_TIMER_10_BIT
#define MOTOR_PWM_MAX_DUTY      1023U

// Task configuration
#define TASK_STACK_DEFAULT      3072
#define TASK_STACK_COMMAND      4096
#define TASK_PRIO_COMMAND       4
#define TASK_PRIO_BINARY        4
#define TASK_PRIO_ANALOG        4
#define TASK_PRIO_OUTPUT        4
#define TASK_PRIO_DISPLAY       2

#endif // APP_CONFIG_H
