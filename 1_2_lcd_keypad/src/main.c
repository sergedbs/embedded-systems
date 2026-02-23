#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

// Include peripheral libraries
#include "lcd_i2c.h"
#include "keypad.h"
#include "led.h"
#include "buzzer.h"
#include "stdio_redirect.h"
#include "config_pins.h"

static const char *TAG = "MAIN";

// Hardcoded PIN for Iteration 3
#define HARDCODED_PIN "1234"

// Lock states
typedef enum {
    STATE_LOCKED,
    STATE_UNLOCKED
} lock_state_t;

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Lock System - Iteration 3 ===");
    ESP_LOGI(TAG, "Initializing peripherals...");
    
    // Initialize all peripherals
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
    lcd_printf("  Iteration 3\n");
    lcd_printf(" Simple Lock/Unlock\n");
    lcd_printf("  Starting...");
    
    // Success beep
    vTaskDelay(pdMS_TO_TICKS(500));
    buzzer_success();
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Lock/Unlock State Machine
    ESP_LOGI(TAG, "Starting lock/unlock state machine...");
    
    lock_state_t state = STATE_LOCKED;
    char pin_input[16];
    
    while (1) {
        switch (state) {
            case STATE_LOCKED:
                // LOCKED state: prompt for PIN
                lcd_clear();
                lcd_printf("=== LOCKED ===\n");
                lcd_printf("\n");
                lcd_printf("Enter PIN:\n");
                lcd_printf("> ");
                
                // Turn off all LEDs
                led_all_off();
                
                // Read PIN from keypad
                lcd_scanf("%15s", pin_input);
                
                ESP_LOGI(TAG, "PIN entered: '%s'", pin_input);
                
                // Validate PIN
                if (strcmp(pin_input, HARDCODED_PIN) == 0) {
                    // Correct PIN - unlock
                    ESP_LOGI(TAG, "Correct PIN - unlocking");
                    
                    lcd_clear();
                    lcd_printf("=== UNLOCKED ===\n");
                    lcd_printf("\n");
                    lcd_printf("  Access Granted!\n");
                    lcd_printf("      \xE2\x9C\x93");  // ✓ checkmark
                    
                    // Success feedback
                    led_success();  // Green LED on
                    buzzer_success();
                    
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    led_all_off();
                    
                    // Transition to UNLOCKED state
                    state = STATE_UNLOCKED;
                    
                } else {
                    // Wrong PIN - stay locked
                    ESP_LOGI(TAG, "Wrong PIN - staying locked");
                    
                    lcd_clear();
                    lcd_printf("=== LOCKED ===\n");
                    lcd_printf("\n");
                    lcd_printf("  Access Denied!\n");
                    lcd_printf("      \xE2\x9C\x97");  // ✗ X mark
                    
                    // Error feedback
                    led_error();  // Red LED on
                    buzzer_error();
                    
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    led_all_off();
                    
                    // Stay in LOCKED state
                }
                break;
                
            case STATE_UNLOCKED:
                // UNLOCKED state: wait for # to lock
                lcd_clear();
                lcd_printf("=== UNLOCKED ===\n");
                lcd_printf("\n");
                lcd_printf("Press # to lock\n");
                
                // Green LED indicates unlocked
                led_set(LED_GREEN, true);
                
                ESP_LOGI(TAG, "Waiting for # to lock...");
                
                // Wait for # key
                while (keypad_getkey_blocking() != '#') {
                    // Wait for lock command
                }
                
                ESP_LOGI(TAG, "Locking...");
                buzzer_beep(100);
                led_all_off();
                
                // Transition to LOCKED state
                state = STATE_LOCKED;
                break;
        }
    }
}
