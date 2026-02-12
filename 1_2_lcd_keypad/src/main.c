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
#include "config_pins.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32 Lock System - Iteration 1 Test ===");
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
    
    ESP_LOGI(TAG, "All peripherals initialized successfully!");
    
    // Display welcome message on LCD
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("  ESP32 Lock System");
    lcd_set_cursor(0, 1);
    lcd_print("    Iteration 1");
    lcd_set_cursor(0, 2);
    lcd_print("  Hardware Test");
    lcd_set_cursor(0, 3);
    lcd_print("Press any key...");
    
    // Success beep
    vTaskDelay(pdMS_TO_TICKS(500));
    buzzer_success();
    
    // Blink LEDs to show all is working
    for (int i = 0; i < 3; i++) {
        led_set(LED_GREEN, true);
        vTaskDelay(pdMS_TO_TICKS(200));
        led_set(LED_GREEN, false);
        led_set(LED_RED, true);
        vTaskDelay(pdMS_TO_TICKS(200));
        led_set(LED_RED, false);
    }
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Main test loop
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Press keys to test");
    lcd_set_cursor(0, 1);
    lcd_print("# = Clear screen");
    lcd_set_cursor(0, 3);
    lcd_print("> ");
    
    ESP_LOGI(TAG, "Entering keypad test loop...");
    
    uint8_t col = 2;
    uint8_t row = 3;
    
    while (1) {
        // Wait for key press
        char key = keypad_getkey_blocking();
        
        ESP_LOGI(TAG, "Key pressed: '%c'", key);
        
        // Play beep on keypress
        buzzer_beep(50);
        
        // Handle special keys
        if (key == '#') {
            // Clear screen
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Press keys to test");
            lcd_set_cursor(0, 1);
            lcd_print("# = Clear screen");
            lcd_set_cursor(0, 3);
            lcd_print("> ");
            col = 2;
            row = 3;
            led_all_off();
        } else if (key == '*') {
            // Backspace
            if (col > 2) {
                col--;
                lcd_set_cursor(col, row);
                lcd_putc(' ');
                lcd_set_cursor(col, row);
            }
        } else if (key == 'A') {
            // Toggle green LED
            static bool green_state = false;
            green_state = !green_state;
            led_set(LED_GREEN, green_state);
        } else if (key == 'B') {
            // Toggle red LED
            static bool red_state = false;
            red_state = !red_state;
            led_set(LED_RED, red_state);
        } else if (key == 'C') {
            // Success pattern
            led_success();
            buzzer_success();
        } else if (key == 'D') {
            // Error pattern
            led_error();
            buzzer_error();
        } else {
            // Normal character - display it
            lcd_set_cursor(col, row);
            lcd_putc(key);
            col++;
            
            // Wrap to next line if needed
            if (col >= LCD_COLS) {
                col = 0;
                row++;
                if (row >= LCD_ROWS) {
                    row = 0;
                }
            }
        }
    }
}
