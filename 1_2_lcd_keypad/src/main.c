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
    
    // Display welcome message on LCD
    lcd_clear();
    lcd_printf("=== LOCK SYSTEM ===\n");
    lcd_printf("\n");
    lcd_printf("\n");
    lcd_printf("  Starting...");
    
    // Success beep
    vTaskDelay(pdMS_TO_TICKS(500));
    buzzer_success();
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Initialize lock system (NVS, load PIN, determine initial state)
    ret = lock_system_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Lock system initialization failed!");
        lcd_clear();
        lcd_printf("ERROR!\n");
        lcd_printf("System init failed\n");
        return;
    }
    
    // Run lock system state machine (never returns)
    lock_system_run();
}
