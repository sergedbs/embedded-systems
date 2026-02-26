#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// GPIO defaults (Wokwi wiring)
#define APP_PIN_BTN         26
#define APP_PIN_LED_GREEN   19
#define APP_PIN_LED_RED     18
#define APP_PIN_LED_YELLOW  21

// Timing configuration
#define APP_TICK_MS              10U
#define APP_TICK_US             (APP_TICK_MS * 1000ULL)
#define APP_DEBOUNCE_TICKS       4U   // 4 * 10ms = 40ms
#define APP_LONG_PRESS_MS        500U
#define APP_LED_FLASH_MS         300U
#define APP_BLINK_INTERVAL_MS     120U

// Derived helpers
#define APP_MS_TO_TICKS(ms)   ((uint32_t)((ms) / APP_TICK_MS))
#define APP_LED_FLASH_TICKS   APP_MS_TO_TICKS(APP_LED_FLASH_MS)
#define APP_BLINK_INTERVAL_TICKS APP_MS_TO_TICKS(APP_BLINK_INTERVAL_MS)

// Reporting interval
#define APP_REPORT_INTERVAL_TICKS  (1000U) // 10s @10ms tick

// Button electrical characteristics
#define APP_BUTTON_ACTIVE_LEVEL    0   // active-low

#endif // APP_CONFIG_H
