/**
 * @file config_pins.h
 * @brief Hardware pin assignments and peripheral configuration
 *
 * Contains GPIO numbers, i2c bus settings, LCD dimensions, and
 * peripheral-level timing constants. Application-level constants
 * (timeouts, PIN policy, UI timing) live in config_app.h.
 */

#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

// ============================================================================
// LCD1602 i2c (16x2, HD44780 via PCF8574 backpack)
// ============================================================================
#define GPIO_LCD_SDA        21
#define GPIO_LCD_SCL        22
#define LCD_I2C_PORT        I2C_NUM_0
#define LCD_I2C_FREQ_HZ     100000      // 100 kHz standard mode
#define LCD_I2C_ADDR_1      0x27        // Default PCF8574 address
#define LCD_I2C_ADDR_2      0x3F        // Alternative (try if 0x27 fails)
#define LCD_COLS            16
#define LCD_ROWS            2

// ============================================================================
// Keypad 4x4 Matrix
// ============================================================================
#define GPIO_KEYPAD_R1      13          // Row GPIOs (outputs)
#define GPIO_KEYPAD_R2      12
#define GPIO_KEYPAD_R3      14
#define GPIO_KEYPAD_R4      27

#define GPIO_KEYPAD_C1      26          // Column GPIOs (inputs + pullup)
#define GPIO_KEYPAD_C2      25
#define GPIO_KEYPAD_C3      33
#define GPIO_KEYPAD_C4      32

#define KEYPAD_ROWS         4
#define KEYPAD_COLS         4
#define KEYPAD_DEBOUNCE_MS  50
#define KEYPAD_SCAN_DELAY_MS 10

// ============================================================================
// LEDs
// ============================================================================
#define GPIO_LED_GREEN      18
#define GPIO_LED_RED        19

// ============================================================================
// Buzzer
// ============================================================================
#define GPIO_BUZZER                 23
#define BUZZER_SUCCESS_DURATION_MS  100
#define BUZZER_ERROR_BEEP_MS        100
#define BUZZER_ERROR_PAUSE_MS       50
#define BUZZER_ERROR_REPEAT         3

#endif // CONFIG_PINS_H
