#include <stdio.h>
#include <string.h>
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

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Lock System - Iteration 2 Test ===");
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
    lcd_printf("  ESP32 Lock System\n");
    lcd_printf("    Iteration 2\n");
    lcd_printf("  STDIO Redirect\n");
    lcd_printf("Press any key...");
    
    // Success beep and LED blink
    vTaskDelay(pdMS_TO_TICKS(500));
    buzzer_success();
    
    for (int i = 0; i < 2; i++) {
        led_set(LED_GREEN, true);
        vTaskDelay(pdMS_TO_TICKS(200));
        led_set(LED_GREEN, false);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // STDIO Redirection Test Loop
    ESP_LOGI(TAG, "Starting STDIO redirection test...");
    
    while (1) {
        char input[16];
        
        lcd_clear();
        lcd_printf("Type something:\n");
        lcd_printf("(* = backspace)\n");
        lcd_printf("(# = enter)\n");
        lcd_printf("> ");
        
        // Read user input via keypad using lcd_scanf
        lcd_scanf("%15s", input);
        
        ESP_LOGI(TAG, "User entered: '%s'", input);
        
        // Display what user typed
        lcd_clear();
        lcd_printf("You typed:\n");
        lcd_printf("%s\n", input);
        lcd_printf("\n");
        lcd_printf("Press # to continue");
        
        // Feedback: success beep and green LED
        led_set(LED_GREEN, true);
        buzzer_success();
        vTaskDelay(pdMS_TO_TICKS(100));
        led_set(LED_GREEN, false);
        
        // Wait for user to press # to continue
        while (keypad_getkey_blocking() != '#') {
            // Wait for # key
        }
        
        buzzer_beep(50);
    }
}
