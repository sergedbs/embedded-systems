/**
 * @file lock_system.c
 * @brief Lock system implementation
 */

#include "lock_system.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "lcd_i2c.h"
#include "keypad.h"
#include "led.h"
#include "buzzer.h"
#include "stdio_redirect.h"
#include "config_pins.h"

static const char *TAG = "LOCK_SYSTEM";

// Lock states
typedef enum {
    STATE_FIRST_BOOT_SETUP,    // Setup wizard - set initial PIN
    STATE_SETTING_CODE,        // User entering new PIN
    STATE_CONFIRMING_CODE,     // User confirming new PIN
    STATE_LOCKED,              // System locked - waiting for PIN
    STATE_UNLOCKED             // System unlocked - show menu
} lock_state_t;

// Global state variables
static lock_state_t current_state;
static char current_pin[PIN_MAX_LENGTH + 1];  // +1 for null terminator
static char new_pin[PIN_MAX_LENGTH + 1];

// ============================================================================
// NVS Storage Functions
// ============================================================================

/**
 * @brief Load PIN from NVS
 * 
 * @param pin_buf Buffer to store PIN (must be at least PIN_MAX_LENGTH + 1)
 * @param buf_size Size of buffer
 * @return true if PIN exists in NVS, false otherwise
 */
static bool load_pin_from_nvs(char *pin_buf, size_t buf_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for reading: %s", esp_err_to_name(err));
        return false;
    }
    
    // Try to read PIN
    size_t required_size = buf_size;
    err = nvs_get_str(nvs_handle, NVS_PIN_KEY, pin_buf, &required_size);
    
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "PIN loaded from NVS (length: %d)", strlen(pin_buf));
        return true;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No PIN found in NVS - first boot");
        return false;
    } else {
        ESP_LOGE(TAG, "Error reading PIN from NVS: %s", esp_err_to_name(err));
        return false;
    }
}

/**
 * @brief Save PIN to NVS
 * 
 * @param pin PIN string to save (4-8 digits)
 * @return ESP_OK on success, error code otherwise
 */
static esp_err_t save_pin_to_nvs(const char *pin)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Validate PIN length
    size_t pin_len = strlen(pin);
    if (pin_len < PIN_MIN_LENGTH || pin_len > PIN_MAX_LENGTH) {
        ESP_LOGE(TAG, "Invalid PIN length: %d (must be %d-%d)", 
                 pin_len, PIN_MIN_LENGTH, PIN_MAX_LENGTH);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write PIN
    err = nvs_set_str(nvs_handle, NVS_PIN_KEY, pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write PIN to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "PIN saved to NVS successfully");
    return ESP_OK;
}

// ============================================================================
// State Handlers
// ============================================================================

/**
 * @brief Handle first boot setup state
 * @return Next state
 */
static lock_state_t handle_first_boot_setup(void)
{
    lcd_clear();
    lcd_printf("===================\n");
    lcd_printf(" SETUP WIZARD\n");
    lcd_printf("===================\n");
    lcd_printf("First boot detected");
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    return STATE_SETTING_CODE;
}

/**
 * @brief Handle setting new code state
 * @return Next state
 */
static lock_state_t handle_setting_code(void)
{
    // Prompt for new PIN
    lcd_clear();
    lcd_printf("Set PIN\n");
    lcd_printf("(%d-%d digits)\n", PIN_MIN_LENGTH, PIN_MAX_LENGTH);
    lcd_printf("\n");
    lcd_printf("> ");
    
    // Read new PIN
    lcd_scanf("%8s", new_pin);
    
    // Validate PIN length
    size_t pin_len = strlen(new_pin);
    if (pin_len < PIN_MIN_LENGTH || pin_len > PIN_MAX_LENGTH) {
        lcd_clear();
        lcd_printf("ERROR!\n");
        lcd_printf("\n");
        lcd_printf("PIN must be\n");
        lcd_printf("%d-%d digits", PIN_MIN_LENGTH, PIN_MAX_LENGTH);
        
        buzzer_error();
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        return STATE_SETTING_CODE;  // Retry
    }
    
    // Validate PIN contains only digits
    bool valid = true;
    for (size_t i = 0; i < pin_len; i++) {
        if (new_pin[i] < '0' || new_pin[i] > '9') {
            valid = false;
            break;
        }
    }
    
    if (!valid) {
        lcd_clear();
        lcd_printf("ERROR!\n");
        lcd_printf("\n");
        lcd_printf("PIN must contain\n");
        lcd_printf("only digits 0-9");
        
        buzzer_error();
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        return STATE_SETTING_CODE;  // Retry
    }
    
    ESP_LOGI(TAG, "New PIN entered (length: %d)", pin_len);
    
    // Show confirmation prompt
    lcd_clear();
    lcd_printf("PIN set!\n");
    lcd_printf("\n");
    lcd_printf("Please confirm...\n");
    
    buzzer_beep(100);
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    return STATE_CONFIRMING_CODE;
}

/**
 * @brief Handle confirming code state
 * @return Next state
 */
static lock_state_t handle_confirming_code(void)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
    // Prompt for PIN confirmation
    lcd_clear();
    lcd_printf("Confirm PIN\n");
    lcd_printf("\n");
    lcd_printf("\n");
    lcd_printf("> ");
    
    // Read confirmation
    lcd_scanf("%8s", pin_input);
    
    ESP_LOGI(TAG, "Confirmation entered");
    
    // Check if PINs match
    if (strcmp(new_pin, pin_input) == 0) {
        // Match - save to NVS
        ESP_LOGI(TAG, "PINs match - saving to NVS");
        
        esp_err_t ret = save_pin_to_nvs(new_pin);
        if (ret == ESP_OK) {
            // Copy to current_pin
            strcpy(current_pin, new_pin);
            
            lcd_clear();
            lcd_printf("SUCCESS!\n");
            lcd_printf("\n");
            lcd_printf("PIN saved\n");
            lcd_printf("    \xE2\x9C\x93");  // ✓
            
            led_success();
            buzzer_success();
            vTaskDelay(pdMS_TO_TICKS(2000));
            led_all_off();
            
            return STATE_LOCKED;
        } else {
            lcd_clear();
            lcd_printf("ERROR!\n");
            lcd_printf("\n");
            lcd_printf("Failed to save\n");
            lcd_printf("Try again...");
            
            led_error();
            buzzer_error();
            vTaskDelay(pdMS_TO_TICKS(2000));
            led_all_off();
            
            return STATE_SETTING_CODE;
        }
    } else {
        // Mismatch - retry
        ESP_LOGI(TAG, "PINs don't match - retry");
        
        lcd_clear();
        lcd_printf("ERROR!\n");
        lcd_printf("\n");
        lcd_printf("PINs don't match\n");
        lcd_printf("Try again...");
        
        led_error();
        buzzer_error();
        vTaskDelay(pdMS_TO_TICKS(2000));
        led_all_off();
        
        return STATE_SETTING_CODE;
    }
}

/**
 * @brief Handle locked state
 * @return Next state
 */
static lock_state_t handle_locked(void)
{
    char pin_input[PIN_MAX_LENGTH + 1];
    
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
    if (strcmp(pin_input, current_pin) == 0) {
        // Correct PIN - unlock
        ESP_LOGI(TAG, "Correct PIN - unlocking");
        
        lcd_clear();
        lcd_printf("=== UNLOCKED ===\n");
        lcd_printf("\n");
        lcd_printf("  Access Granted!\n");
        lcd_printf("      \xE2\x9C\x93");  // ✓ checkmark
        
        // Success feedback
        led_success();
        buzzer_success();
        
        vTaskDelay(pdMS_TO_TICKS(1500));
        led_all_off();
        
        return STATE_UNLOCKED;
    } else {
        // Wrong PIN - stay locked
        ESP_LOGI(TAG, "Wrong PIN - staying locked");
        
        lcd_clear();
        lcd_printf("=== LOCKED ===\n");
        lcd_printf("\n");
        lcd_printf("  Access Denied!\n");
        lcd_printf("      \xE2\x9C\x97");  // ✗ X mark
        
        // Error feedback
        led_error();
        buzzer_error();
        
        vTaskDelay(pdMS_TO_TICKS(1500));
        led_all_off();
        
        return STATE_LOCKED;
    }
}

/**
 * @brief Handle unlocked state
 * @return Next state
 */
static lock_state_t handle_unlocked(void)
{
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
    
    return STATE_LOCKED;
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t lock_system_init(void)
{
    ESP_LOGI(TAG, "Initializing lock system...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "NVS initialized");
    
    // Check if PIN exists in NVS
    if (load_pin_from_nvs(current_pin, sizeof(current_pin))) {
        // PIN exists - go to locked state
        ESP_LOGI(TAG, "PIN found - starting in LOCKED state");
        current_state = STATE_LOCKED;
    } else {
        // No PIN - first boot setup
        ESP_LOGI(TAG, "No PIN found - starting SETUP WIZARD");
        current_state = STATE_FIRST_BOOT_SETUP;
    }
    
    return ESP_OK;
}

void lock_system_run(void)
{
    ESP_LOGI(TAG, "Starting lock system state machine...");
    
    // Main state machine loop
    while (1) {
        switch (current_state) {
            case STATE_FIRST_BOOT_SETUP:
                current_state = handle_first_boot_setup();
                break;
                
            case STATE_SETTING_CODE:
                current_state = handle_setting_code();
                break;
                
            case STATE_CONFIRMING_CODE:
                current_state = handle_confirming_code();
                break;
                
            case STATE_LOCKED:
                current_state = handle_locked();
                break;
                
            case STATE_UNLOCKED:
                current_state = handle_unlocked();
                break;
                
            default:
                ESP_LOGE(TAG, "Unknown state: %d", current_state);
                current_state = STATE_LOCKED;
                break;
        }
    }
}
