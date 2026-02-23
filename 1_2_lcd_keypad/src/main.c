/**
 * @file main.c
 * @brief ESP32 Lock System - Main entry point
 * 
 * Initializes all peripherals and starts the lock system.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Peripheral libraries
#include "lcd_i2c.h"
#include "keypad.h"
#include "led.h"
#include "buzzer.h"
#include "stdio_redirect.h"

// Lock system
#include "lock_system.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Lock System ===");
    ESP_LOGI(TAG, "Initializing peripherals...");
    
    esp_err_t ret;
    
    // Initialize LCD
    ret = lcd_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LCD initialization failed!");
        return;
    }
    
    // Initialize Keypad
    ret = keypad_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Keypad initialization failed!");
        return;
    }
    
    // Initialize LEDs
    ret = led_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED initialization failed!");
        return;
    }
    
    // Initialize Buzzer
    ret = buzzer_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer initialization failed!");
        return;
    }
    
    // Initialize STDIO redirection
    ret = stdio_redirect_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "STDIO redirection initialization failed!");
        return;
    }
    
    ESP_LOGI(TAG, "All peripherals initialized successfully!");

    // Boot complete signal
    buzzer_success();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize lock system (NVS, load PIN, determine initial state)
    ret = lock_system_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Lock system initialization failed!");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("ERROR!");
        lcd_set_cursor(0, 1);
        lcd_print("Init failed");
        return;
    }
    
    // Run lock system state machine (never returns)
    lock_system_run();
}
