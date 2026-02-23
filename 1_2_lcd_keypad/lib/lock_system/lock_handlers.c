/**
 * @file lock_handlers.c
 * @brief State handler functions for lock system
 */

#include "lock_handlers.h"
#include "lock_storage.h"
#include "lock_ui.h"
#include "lock_system.h"
#include "lcd_i2c.h"
#include "keypad.h"
#include "led.h"
#include "buzzer.h"
#include "stdio_redirect.h"
#include "config_pins.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "LOCK_HANDLERS";

lock_state_t lock_handler_first_boot_setup(lock_state_context_t *ctx)
{
    lock_ui_display_title("SETUP WIZARD");
    lcd_set_cursor(0, 3);
    lcd_printf("First boot detected");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Set return state for after PIN confirmation
    *ctx->return_state = STATE_LOCKED;
    
    return STATE_SETTING_CODE;
}

lock_state_t lock_handler_setting_code(lock_state_context_t *ctx)
{
    // Prompt for new PIN with centered UI
    lcd_clear();
    lock_ui_display_centered("Set PIN", 0);
    lock_ui_display_centered("(4-8 digits)", 1);
    lcd_set_cursor(0, 3);
    lcd_printf("Enter: ");
    
    // Configure: reveal last char + digits only
    lcd_scanf_configure(INPUT_MODE_REVEAL_LAST, true);
    
    // Read new PIN
    lcd_scanf("%8s", ctx->new_pin);
    
    // Validate PIN length
    size_t pin_len = strlen(ctx->new_pin);
    if (pin_len < PIN_MIN_LENGTH || pin_len > PIN_MAX_LENGTH) {
        lock_ui_display_error("PIN must be", 0);
        lock_ui_display_centered("4-8 digits", 3);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return STATE_SETTING_CODE;  // Retry
    }
    
    // Validate PIN contains only digits
    for (size_t i = 0; i < pin_len; i++) {
        if (ctx->new_pin[i] < '0' || ctx->new_pin[i] > '9') {
            lock_ui_display_error("Only digits 0-9", 2000);
            return STATE_SETTING_CODE;  // Retry
        }
    }
    
    ESP_LOGI(TAG, "New PIN entered (length: %d)", pin_len);
    
    // Show confirmation prompt
    lcd_clear();
    lock_ui_display_centered("PIN set!", 1);
    lock_ui_display_centered("Please confirm...", 2);
    
    buzzer_beep(100);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    return STATE_CONFIRMING_CODE;
}

lock_state_t lock_handler_confirming_code(lock_state_context_t *ctx)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // Prompt for PIN confirmation with centered UI
    lcd_clear();
    lock_ui_display_centered("Confirm PIN", 1);
    lcd_set_cursor(0, 3);
    lcd_printf("Enter: ");
    
    // Configure: reveal last char + digits only
    lcd_scanf_configure(INPUT_MODE_REVEAL_LAST, true);
    
    // Read confirmation
    lcd_scanf("%8s", pin_input);
    
    ESP_LOGI(TAG, "Confirmation entered");
    
    // Check if PINs match
    if (strcmp(ctx->new_pin, pin_input) == 0) {
        // Match - save to NVS
        ESP_LOGI(TAG, "PINs match - saving to NVS");
        
        esp_err_t ret = lock_storage_save_pin(ctx->new_pin);
        if (ret == ESP_OK) {
            // Copy to current_pin
            strcpy(ctx->current_pin, ctx->new_pin);
            
            lock_ui_display_success("PIN saved", 0);
            lock_ui_display_centered("\xE2\x9C\x93", 3);  // ✓
            vTaskDelay(pdMS_TO_TICKS(2000));
            
            // Restart auto-lock if returning to menu
            if (*ctx->return_state == STATE_MENU) {
                lock_system_start_autolock();
            }
            
            // Return to appropriate state (LOCKED for first setup, MENU for change)
            return *ctx->return_state;
        } else {
            lock_ui_display_error("Failed to save", 0);
            lock_ui_display_centered("Try again...", 3);
            vTaskDelay(pdMS_TO_TICKS(2000));
            return STATE_SETTING_CODE;
        }
    } else {
        // Mismatch - retry
        ESP_LOGI(TAG, "PINs don't match - retry");
        lock_ui_display_error("PINs don't match", 0);
        lock_ui_display_centered("Try again...", 3);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return STATE_SETTING_CODE;
    }
}

lock_state_t lock_handler_locked(lock_state_context_t *ctx)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // LOCKED state: prompt for PIN with centered UI
    lock_ui_display_title("LOCKED");
    lcd_set_cursor(0, 3);
    lcd_printf("Enter PIN: ");
    
    // Turn off all LEDs
    led_all_off();
    
    // Configure: masked input + digits only
    lcd_scanf_configure(INPUT_MODE_MASKED, true);
    
    // Read PIN from keypad
    lcd_scanf("%15s", pin_input);
    
    ESP_LOGI(TAG, "PIN entered");
    
    // Validate PIN
    if (strcmp(pin_input, ctx->current_pin) == 0) {
        // Correct PIN - unlock
        ESP_LOGI(TAG, "Correct PIN - unlocking");
        
        // Display success message (no title to prevent double refresh)
        lock_ui_display_success("Access Granted!", 1500);
        
        return STATE_UNLOCKED;
    } else {
        // Wrong PIN - stay locked
        ESP_LOGI(TAG, "Wrong PIN - staying locked");
        
        lock_ui_display_title("LOCKED");
        lock_ui_display_centered("Access Denied!", 3);
        
        // Error feedback
        led_error();
        buzzer_error();
        
        vTaskDelay(pdMS_TO_TICKS(1500));
        led_all_off();
        
        return STATE_LOCKED;
    }
}

lock_state_t lock_handler_unlocked(lock_state_context_t *ctx)
{
    // UNLOCKED state: transition directly to menu
    // (success message already shown by lock_handler_locked)
    
    // Green LED stays on from previous state
    
    // Start auto-lock timer when entering menu
    lock_system_start_autolock();
    
    return STATE_MENU;
}

lock_state_t lock_handler_menu(lock_state_context_t *ctx)
{
    lock_ui_display_title("MENU");
    lcd_set_cursor(0, 3);
    lcd_printf("A. Change  D. Lock");
    
    // Green LED stays on
    led_set(LED_GREEN, true);
    
    ESP_LOGI(TAG, "Menu displayed - waiting for selection...");
    
    // Wait for valid input
    char key;
    while (1) {
        key = keypad_getkey_blocking();
        
        // Reset auto-lock timer on any keypress
        lock_system_reset_autolock();
        
        if (key == 'D') {
            // Lock system
            ESP_LOGI(TAG, "User selected: Lock");
            lock_system_stop_autolock();
            lock_ui_display_centered("Locking...", 3);
            buzzer_beep(100);
            vTaskDelay(pdMS_TO_TICKS(500));
            led_all_off();
            return STATE_LOCKED;
            
        } else if (key == 'A') {
            // Change PIN
            ESP_LOGI(TAG, "User selected: Change PIN");
            lock_system_stop_autolock();
            buzzer_beep(100);
            return STATE_CHANGING_CODE;
            
        } else {
            // Invalid selection - beep error
            buzzer_beep(50);
        }
    }
}

lock_state_t lock_handler_changing_code(lock_state_context_t *ctx)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // Prompt for current PIN with centered UI
    lcd_clear();
    lock_ui_display_centered("Change PIN", 0);
    lcd_printf("\n");
    lock_ui_display_centered("Enter current PIN:", 2);
    lcd_set_cursor(0, 3);
    lcd_printf("Enter: ");
    
    // Configure: masked input + digits only
    lcd_scanf_configure(INPUT_MODE_MASKED, true);
    
    // Read current PIN
    lcd_scanf("%8s", pin_input);
    
    ESP_LOGI(TAG, "Current PIN entered for validation");
    
    // Validate current PIN
    if (strcmp(pin_input, ctx->current_pin) == 0) {
        // Correct - proceed to setting new PIN
        ESP_LOGI(TAG, "Current PIN correct - allow change");
        
        lcd_clear();
        lock_ui_display_centered("Verified!", 1);
        lock_ui_display_centered("\xE2\x9C\x93", 2);  // ✓
        
        buzzer_success();
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Set return state to menu after change complete
        *ctx->return_state = STATE_MENU;
        return STATE_SETTING_CODE;
        
    } else {
        // Wrong PIN - return to menu
        ESP_LOGI(TAG, "Wrong current PIN - return to menu");
        
        lock_ui_display_error("Wrong PIN", 0);
        lock_ui_display_centered("Try again...", 3);
        vTaskDelay(pdMS_TO_TICKS(2000));
        led_all_off();
        
        // Restart auto-lock when returning to menu
        lock_system_start_autolock();
        
        return STATE_MENU;
    }
}
