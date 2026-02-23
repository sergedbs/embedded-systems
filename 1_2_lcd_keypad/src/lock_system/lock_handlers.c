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
#include "mbedtls/sha256.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

static const char *TAG = "LOCK_HANDLERS";

// ---------------------------------------------------------------------------
// Security helpers
// ---------------------------------------------------------------------------

/**
 * @brief Compute SHA-256 of a plain-text PIN and write the 64-char lower-hex
 *        digest (+ NUL terminator) into out[65].
 */
static void hash_pin(const char *pin, char out[65])
{
    unsigned char digest[32];
    mbedtls_sha256((const unsigned char *)pin, strlen(pin), digest, 0);
    for (int i = 0; i < 32; i++) {
        sprintf(out + i * 2, "%02x", digest[i]);
    }
    out[64] = '\0';
}

/**
 * @brief Constant-time comparison of two byte strings of known length.
 *
 * Avoids timing side-channels caused by strcmp()'s early-exit behaviour.
 * Returns true only when every byte is identical.
 */
static bool ct_str_equal(const char *a, const char *b, size_t len)
{
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) {
        diff |= (uint8_t)(a[i] ^ b[i]);
    }
    return diff == 0;
}

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
    printf("Pin: ");

    // Configure: reveal last char + digits only
    set_input_mode(INPUT_MODE_REVEAL_LAST, true);

    // Read new PIN
    scanf("%8s", ctx->new_pin);
    
    // Check for cancel
    if (stdin_was_cancelled()) {
        if (*ctx->return_state == STATE_MENU) {
            ESP_LOGI(TAG, "PIN entry cancelled");
            lcd_clear();
            lock_ui_display_centered("Cancelled", 0);
            buzzer_beep(100);
            vTaskDelay(pdMS_TO_TICKS(1000));
            lock_system_start_autolock();
            return STATE_MENU;
        } else {
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
    printf("Pin: ");

    // Configure: reveal last char + digits only
    set_input_mode(INPUT_MODE_REVEAL_LAST, true);

    // Read confirmation
    scanf("%8s", pin_input);

    // Check for cancel
    if (stdin_was_cancelled()) {
        ESP_LOGI(TAG, "PIN confirmation cancelled");
        lcd_clear();
        lock_ui_display_centered("Cancelled", 0);
        buzzer_beep(100);
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Return to setting code state to re-enter PIN
        return STATE_SETTING_CODE;
    }

    // Check if PINs match
    if (strcmp(ctx->new_pin, pin_input) == 0) {
        ESP_LOGI(TAG, "PINs match - hashing and saving to NVS");

        // Hash the plain-text PIN; only the digest is persisted
        char pin_hash[65];
        hash_pin(ctx->new_pin, pin_hash);

        esp_err_t ret = lock_storage_save_pin(pin_hash);
        if (ret == ESP_OK) {
            strcpy(ctx->current_pin, pin_hash);

            lock_ui_display_success("Saved!", 1500);

            if (*ctx->return_state == STATE_MENU) {
                lock_system_start_autolock();
            }

            return *ctx->return_state;
        } else {
            lock_ui_display_error("Save failed", 0);
            vTaskDelay(pdMS_TO_TICKS(2000));
            return STATE_SETTING_CODE;
        }
    } else {
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
        lock_ui_display_centered("(15s idle)", 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
        *ctx->was_auto_locked = false;  // Clear flag
    }
    
    // Turn off all LEDs
    led_all_off();
    
    // Loop: re-prompt on 'C' clear without counting as a failed attempt
    while (1) {
        // Display LOCKED title and PIN prompt (16x2 layout)
        lock_ui_display_title("LOCKED");
        lcd_clear_row(1);
        lcd_set_cursor(0, 1);
        printf("PIN: ");

        // Configure: masked input + digits only
        set_input_mode(INPUT_MODE_MASKED, true);

        // Read PIN from keypad
        scanf("%8s", pin_input);
        
        // 'C' pressed: clear current input and re-prompt silently
        if (stdin_was_cancelled()) {
            ESP_LOGI(TAG, "PIN input cleared");
            continue;
        }

        // Validate PIN
        char pin_hash[65];
        hash_pin(pin_input, pin_hash);

        if (ct_str_equal(pin_hash, ctx->current_pin, 64)) {
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
            ESP_LOGI(TAG, "Wrong PIN - attempt %d/%d", *ctx->failed_attempts, MAX_FAILED_ATTEMPTS);
            
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
    // UNLOCKED state: transition directly to menu.
    // (success message + led_all_off already called by lock_handler_locked)
    // lock_handler_menu re-enables the green LED on entry.
    
    // Start auto-lock timer when entering menu
    lock_system_start_autolock();
    
    return STATE_MENU;
}

lock_state_t lock_handler_menu(lock_state_context_t *ctx)
{
    lock_ui_display_title("MENU");
    lcd_clear_row(1);
    lcd_set_cursor(0, 1);
    printf("A:Chg  D:Lock");

    // Green LED stays on
    led_set(LED_GREEN, true);
    
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
                ESP_LOGI(TAG, "User selected: Lock");
                lock_system_stop_autolock();
                lcd_clear_row(1);
                lock_ui_display_centered("Locking...", 1);
                buzzer_beep(100);
                vTaskDelay(pdMS_TO_TICKS(500));
                led_all_off();
                return STATE_LOCKED;
                
            } else if (key == 'A') {
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
    printf("Old: ");

    // Configure: masked input + digits only
    set_input_mode(INPUT_MODE_MASKED, true);

    // Read current PIN
    scanf("%8s", pin_input);
    
    // Check for cancel
    if (stdin_was_cancelled()) {
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
    char pin_hash[65];
    hash_pin(pin_input, pin_hash);

    if (ct_str_equal(pin_hash, ctx->current_pin, 64)) {
        ESP_LOGI(TAG, "Current PIN correct - allow change");
        lcd_clear();
        lock_ui_display_centered("Verified!", 0);
        lcd_clear_row(1);
        
        buzzer_success();
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Set return state to menu after change complete
        *ctx->return_state = STATE_MENU;
        return STATE_SETTING_CODE;
        
    } else {
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
        printf("Wait %2ds...    ", remaining);
        
        if (remaining > 0) {
            // Poll keypad each second so keypresses can restore the backlight.
            // The key itself is discarded — lockout is not cancellable.
            for (int ms = 0; ms < 1000; ms += 50) {
                char k = keypad_getkey();
                if (k != '\0') {
                    lock_system_reset_backlight_timer();
                }
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
    }
    
    // Reset failed attempts counter
    *ctx->failed_attempts = 0;
    
    // Turn off LED
    led_all_off();
    
    ESP_LOGI(TAG, "Lockout complete - returning to LOCKED");
    
    return STATE_LOCKED;
}
