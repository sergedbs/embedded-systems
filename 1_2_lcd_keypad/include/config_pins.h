#ifndef CONFIG_PINS_H
#define CONFIG_PINS_H

#include <stdint.h>

// ============================================================================
// LCD2004 i2c Configuration
// ============================================================================
#define GPIO_LCD_SDA        21
#define GPIO_LCD_SCL        22
#define LCD_I2C_PORT        I2C_NUM_0
#define LCD_I2C_FREQ_HZ     100000      // 100kHz standard mode
#define LCD_I2C_ADDR_1      0x27        // Default PCF8574 address
#define LCD_I2C_ADDR_2      0x3F        // Alternative address (try if 0x27 fails)
#define LCD_COLS            20          // 20 columns
#define LCD_ROWS            4           // 4 rows

// ============================================================================
// Keypad 4x4 Matrix Configuration
// ============================================================================
// Row GPIOs (outputs - scan rows)
#define GPIO_KEYPAD_R1      13
#define GPIO_KEYPAD_R2      12
#define GPIO_KEYPAD_R3      14
#define GPIO_KEYPAD_R4      27

// Column GPIOs (inputs with pullup - read columns)
#define GPIO_KEYPAD_C1      26
#define GPIO_KEYPAD_C2      25
#define GPIO_KEYPAD_C3      33
#define GPIO_KEYPAD_C4      32

#define KEYPAD_ROWS         4
#define KEYPAD_COLS         4

// Keypad timing constants
#define KEYPAD_DEBOUNCE_MS  50          // Debounce delay in milliseconds
#define KEYPAD_SCAN_DELAY_MS 10         // Delay between scans

// ============================================================================
// LED Configuration
// ============================================================================
#define GPIO_LED_GREEN      18
#define GPIO_LED_RED        19

// ============================================================================
// Buzzer Configuration
// ============================================================================
#define GPIO_BUZZER         23

// Buzzer timing constants
#define BUZZER_SUCCESS_DURATION_MS  100
#define BUZZER_ERROR_BEEP_MS        100
#define BUZZER_ERROR_PAUSE_MS       50
#define BUZZER_ERROR_REPEAT         3

// ============================================================================
// Application Timing Constants
// ============================================================================
#define AUTOLOCK_TIMEOUT_SEC    30      // Auto-lock after 30 seconds
#define CHAR_REVEAL_MS          500     // Show last char for 500ms when entering new PIN
#define PIN_MIN_LENGTH          4       // Minimum PIN length
#define PIN_MAX_LENGTH          8       // Maximum PIN length

// ============================================================================
// NVS Storage Configuration
// ============================================================================
#define NVS_NAMESPACE       "storage"
#define NVS_PIN_KEY         "user_pin"

#endif // CONFIG_PINS_H
