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
#include "config_app.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "LOCK_HANDLERS";

lock_state_t lock_handler_first_boot_setup(lock_state_context_t *ctx)
{
    // Welcome message (16x2 layout)
    lcd_clear();
    lock_ui_display_centered("SECURITY LOCK", 0);
    lock_ui_display_centered("v1.0", 1);
    
    buzzer_beep(100);
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Setup wizard
    lock_ui_display_title("SETUP WIZARD");
    lock_ui_display_centered("1st boot", 1);
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Set return state for after PIN confirmation
    *ctx->return_state = STATE_LOCKED;
    
    return STATE_SETTING_CODE;
}

lock_state_t lock_handler_setting_code(lock_state_context_t *ctx)
{
    // Prompt for new PIN (16x2 layout)
    lcd_clear();
    
    // Show cancel hint if this is a PIN change (not first boot)
    if (*ctx->return_state == STATE_MENU) {
        lock_ui_display_centered("New PIN (C=X)", 0);
    } else {
        lock_ui_display_centered("Set PIN (4-8)", 0);
    }
    
    lcd_set_cursor(0, 1);
    lcd_printf("Pin: ");
    
    // Configure: reveal last char + digits only
    lcd_scanf_configure(INPUT_MODE_REVEAL_LAST, true);
    
    // Read new PIN
    int result = lcd_scanf("%8s", ctx->new_pin);
    
    // Check for cancel
    if (result == -1) {
        if (*ctx->return_state == STATE_MENU) {
            // Changing PIN (cancelable) - show message, return to menu
            ESP_LOGI(TAG, "PIN entry cancelled");
            lcd_clear();
            lock_ui_display_centered("Cancelled", 0);
            buzzer_beep(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
            lock_system_start_autolock();
            return STATE_MENU;
        } else {
            // First boot setup (not cancelable) - re-prompt silently
            ESP_LOGI(TAG, "Cancel ignored - first boot setup");
            return STATE_SETTING_CODE;
        }
    }
    
    // Validate PIN length
    size_t pin_len = strlen(ctx->new_pin);
    if (pin_len < PIN_MIN_LENGTH || pin_len > PIN_MAX_LENGTH) {
        lock_ui_display_title("ERROR");
        lock_ui_display_centered("Must be 4-8", 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return STATE_SETTING_CODE;  // Retry
    }
    
    // Validate PIN contains only digits
    for (size_t i = 0; i < pin_len; i++) {
        if (ctx->new_pin[i] < '0' || ctx->new_pin[i] > '9') {
            lock_ui_display_error("Digits only", 2000);
            return STATE_SETTING_CODE;  // Retry
        }
    }
    
    ESP_LOGI(TAG, "New PIN entered (length: %d)", pin_len);
    
    // Show confirmation prompt
    lcd_clear();
    lock_ui_display_centered("PIN set!", 0);
    lock_ui_display_centered("Confirm...", 1);
    
    buzzer_beep(100);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    return STATE_CONFIRMING_CODE;
}

lock_state_t lock_handler_confirming_code(lock_state_context_t *ctx)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // Prompt for PIN confirmation (16x2 layout)
    lcd_clear();
    lock_ui_display_centered("Confirm PIN", 0);
    lcd_set_cursor(0, 1);
    lcd_printf("Pin: ");
    
    // Configure: reveal last char + digits only
    lcd_scanf_configure(INPUT_MODE_REVEAL_LAST, true);
    
    // Read confirmation
    int result = lcd_scanf("%8s", pin_input);
    
    // Check for cancel
    if (result == -1) {
        ESP_LOGI(TAG, "PIN confirmation cancelled");
        lcd_clear();
        lock_ui_display_centered("Cancelled", 0);
        buzzer_beep(100);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Return to setting code state to re-enter PIN
        return STATE_SETTING_CODE;
    }
    
    ESP_LOGI(TAG, "Confirmation entered");
    
    // Check if PINs match
    if (strcmp(ctx->new_pin, pin_input) == 0) {
        // Match - save to NVS
        ESP_LOGI(TAG, "PINs match - saving to NVS");
        
        esp_err_t ret = lock_storage_save_pin(ctx->new_pin);
        if (ret == ESP_OK) {
            // Copy to current_pin
            strcpy(ctx->current_pin, ctx->new_pin);
            
            lock_ui_display_success("Saved!", 0);
            vTaskDelay(pdMS_TO_TICKS(1500));
            
            // Restart auto-lock if returning to menu
            if (*ctx->return_state == STATE_MENU) {
                lock_system_start_autolock();
            }
            
            // Return to appropriate state (LOCKED for first setup, MENU for change)
            return *ctx->return_state;
        } else {
            lock_ui_display_error("Save failed", 0);
            vTaskDelay(pdMS_TO_TICKS(2000));
            return STATE_SETTING_CODE;
        }
    } else {
        // Mismatch - retry
        ESP_LOGI(TAG, "PINs don't match - retry");
        lock_ui_display_error("No match!", 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        return STATE_SETTING_CODE;
    }
}

lock_state_t lock_handler_locked(lock_state_context_t *ctx)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // Check if locked by auto-lock timer
    if (*ctx->was_auto_locked) {
        lcd_clear();
        lock_ui_display_centered("AUTO-LOCKED", 0);
        lock_ui_display_centered("(30s idle)", 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
        *ctx->was_auto_locked = false;  // Clear flag
    }
    
    // Turn off all LEDs
    led_all_off();
    
    // Loop: re-prompt on 'C' clear without counting as a failed attempt
    while (1) {
        // Display LOCKED title and PIN prompt (16x2 layout)
        lock_ui_display_title("LOCKED");
        lock_ui_clear_row(1);
        lcd_set_cursor(0, 1);
        lcd_printf("PIN: ");
        
        // Configure: masked input + digits only
        lcd_scanf_configure(INPUT_MODE_MASKED, true);
        
        // Read PIN from keypad
        int result = lcd_scanf("%15s", pin_input);
        
        // 'C' pressed: clear current input and re-prompt silently
        if (result == -1) {
            ESP_LOGI(TAG, "PIN input cleared");
            continue;
        }
        
        ESP_LOGI(TAG, "PIN entered");
        
        // Validate PIN
        if (strcmp(pin_input, ctx->current_pin) == 0) {
            // Correct PIN - unlock
            ESP_LOGI(TAG, "Correct PIN - unlocking");
            
            // Reset failed attempts counter
            *ctx->failed_attempts = 0;
            
            // Display success message
            lock_ui_display_success("Granted!", 1500);
            
            return STATE_UNLOCKED;
        } else {
            // Wrong PIN - increment failed attempts
            (*ctx->failed_attempts)++;
            ESP_LOGI(TAG, "Wrong PIN - attempt %d/3", *ctx->failed_attempts);
            
            // Check if lockout threshold reached
            if (*ctx->failed_attempts >= MAX_FAILED_ATTEMPTS) {
                ESP_LOGW(TAG, "Too many failed attempts - entering lockout");
                return STATE_LOCKOUT;
            }
            
            // Show error and loop back to re-prompt
            lock_ui_display_title("LOCKED");
            lock_ui_display_centered("Denied!", 1);
            
            led_error();
            buzzer_error();
            
            vTaskDelay(pdMS_TO_TICKS(1500));
            led_all_off();
            // Loop continues - re-prompt without leaving STATE_LOCKED
        }
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
    lock_ui_clear_row(1);  // Clear row 1 before displaying menu options
    lcd_set_cursor(0, 1);
    lcd_printf("A:Chg  D:Lock");
    
    // Green LED stays on
    led_set(LED_GREEN, true);
    
    ESP_LOGI(TAG, "Menu displayed - waiting for selection...");
    
    // Wait for valid input (non-blocking loop to detect timer state changes)
    char key;
    while (1) {
        // Check if auto-lock timer changed state
        if (*ctx->current_state != STATE_MENU) {
            ESP_LOGI(TAG, "State changed by timer - exiting menu");
            led_all_off();
            return *ctx->current_state;
        }
        
        // Non-blocking keypad read
        key = keypad_getkey();
        
        if (key != '\0') {
            // Key pressed - reset timers
            lock_system_reset_backlight_timer();
            lock_system_reset_autolock();
            
            if (key == 'D') {
                // Lock system
                ESP_LOGI(TAG, "User selected: Lock");
                lock_system_stop_autolock();
                lock_ui_clear_row(1);
                lock_ui_display_centered("Locking...", 1);
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
        
        // Small delay to avoid busy-waiting
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

lock_state_t lock_handler_changing_code(lock_state_context_t *ctx)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // Prompt for current PIN (16x2 layout)
    lcd_clear();
    lock_ui_display_centered("Chg PIN (C=X)", 0);
    lcd_set_cursor(0, 1);
    lcd_printf("Old: ");
    
    // Configure: masked input + digits only
    lcd_scanf_configure(INPUT_MODE_MASKED, true);
    
    // Read current PIN
    int result = lcd_scanf("%8s", pin_input);
    
    // Check for cancel
    if (result == -1) {
        ESP_LOGI(TAG, "PIN change cancelled");
        lcd_clear();
        lock_ui_display_centered("Cancelled", 0);
        buzzer_beep(100);
        vTaskDelay(pdMS_TO_TICKS(1000));
        led_all_off();
        
        // Return to menu and restart auto-lock
        lock_system_start_autolock();
        return STATE_MENU;
    }
    
    ESP_LOGI(TAG, "Current PIN entered for validation");
    
    // Validate current PIN
    if (strcmp(pin_input, ctx->current_pin) == 0) {
        // Correct - proceed to setting new PIN
        ESP_LOGI(TAG, "Current PIN correct - allow change");
        
        lcd_clear();
        lock_ui_display_centered("Verified!", 0);
        lock_ui_display_centered("\xE2\x9C\x93", 1);  // ✓
        
        buzzer_success();
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Set return state to menu after change complete
        *ctx->return_state = STATE_MENU;
        return STATE_SETTING_CODE;
        
    } else {
        // Wrong PIN - return to menu
        ESP_LOGI(TAG, "Wrong current PIN - return to menu");
        
        lock_ui_display_error("Wrong PIN!", 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        led_all_off();
        
        // Restart auto-lock when returning to menu
        lock_system_start_autolock();
        
        return STATE_MENU;
    }
}

lock_state_t lock_handler_lockout(lock_state_context_t *ctx)
{
    ESP_LOGW(TAG, "LOCKOUT state - too many failed attempts");
    
    // Display lockout message (16x2 layout)
    lcd_clear();
    lock_ui_display_centered("TOO MANY", 0);
    lock_ui_display_centered("ATTEMPTS!", 1);
    
    // Red LED continuous
    led_set(LED_RED, true);
    
    // Loud buzzer pattern (3 long beeps)
    for (int i = 0; i < 3; i++) {
        buzzer_beep(300);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    // Display title once (no flicker)
    lock_ui_display_title("LOCKED OUT");
    
    // Display countdown (LOCKOUT_DURATION_SEC seconds) - only update row 1
    const int lockout_duration = LOCKOUT_DURATION_SEC;
    for (int remaining = lockout_duration; remaining >= 0; remaining--) {
        // Only update countdown on row 1 (don't redraw title)
        lcd_set_cursor(0, 1);
        lcd_printf("Wait %2ds...    ", remaining);
        
        if (remaining > 0) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    // Reset failed attempts counter
    *ctx->failed_attempts = 0;
    
    // Turn off LED
    led_all_off();
    
    ESP_LOGI(TAG, "Lockout complete - returning to LOCKED");
    
    return STATE_LOCKED;
}
