/**
 * @file lock_system.c
 * @brief Lock system state machine coordination
 */

#include "lock_system.h"
#include "lock_storage.h"
#include "lock_handlers.h"
#include "config_pins.h"
#include "lcd_i2c.h"
#include "led.h"
#include "lock_ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "LOCK_SYSTEM";

// Global state variables
static lock_state_t current_state;
static lock_state_t return_state = STATE_LOCKED;
static char current_pin[PIN_MAX_LENGTH + 1];
static char new_pin[PIN_MAX_LENGTH + 1];

// Auto-lock timer
static TimerHandle_t autolock_timer = NULL;
static bool autolock_enabled = false;

// State context for handlers
static lock_state_context_t state_ctx = {
    .current_pin = current_pin,
    .new_pin = new_pin,
    .return_state = &return_state
};

// ============================================================================
// Auto-lock Timer
// ============================================================================

/**
 * @brief Auto-lock timer callback - transitions to LOCKED state
 */
static void autolock_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Auto-lock timer expired - locking system");
    
    // Turn off LEDs
    led_all_off();
    
    // Display auto-lock message
    lcd_clear();
    lock_ui_display_title("AUTO-LOCKED");
    lock_ui_display_centered("Idle timeout", 3);
    
    vTaskDelay(pdMS_TO_TICKS(1500));
    
    // Transition to locked state
    current_state = STATE_LOCKED;
    autolock_enabled = false;
}

void lock_system_start_autolock(void)
{
    if (autolock_timer == NULL) {
        // Create timer on first use
        autolock_timer = xTimerCreate(
            "autolock",
            pdMS_TO_TICKS(AUTOLOCK_TIMEOUT_SEC * 1000),
            pdFALSE,  // One-shot timer
            NULL,
            autolock_timer_callback
        );
        
        if (autolock_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create auto-lock timer");
            return;
        }
    }
    
    // Start/restart the timer
    if (xTimerStart(autolock_timer, 0) == pdPASS) {
        autolock_enabled = true;
        ESP_LOGI(TAG, "Auto-lock timer started (%d seconds)", AUTOLOCK_TIMEOUT_SEC);
    } else {
        ESP_LOGE(TAG, "Failed to start auto-lock timer");
    }
}

void lock_system_reset_autolock(void)
{
    if (autolock_enabled && autolock_timer != NULL) {
        if (xTimerReset(autolock_timer, 0) == pdPASS) {
            ESP_LOGD(TAG, "Auto-lock timer reset");
        }
    }
}

void lock_system_stop_autolock(void)
{
    if (autolock_timer != NULL && autolock_enabled) {
        xTimerStop(autolock_timer, 0);
        autolock_enabled = false;
        ESP_LOGI(TAG, "Auto-lock timer stopped");
    }
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t lock_system_init(void)
{
    ESP_LOGI(TAG, "Initializing lock system...");
    
    // Initialize NVS storage
    esp_err_t ret = lock_storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize storage: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Check if PIN exists in NVS
    if (lock_storage_load_pin(current_pin, sizeof(current_pin))) {
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
                current_state = lock_handler_first_boot_setup(&state_ctx);
                break;
                
            case STATE_SETTING_CODE:
                current_state = lock_handler_setting_code(&state_ctx);
                break;
                
            case STATE_CONFIRMING_CODE:
                current_state = lock_handler_confirming_code(&state_ctx);
                break;
                
            case STATE_LOCKED:
                current_state = lock_handler_locked(&state_ctx);
                break;
                
            case STATE_UNLOCKED:
                current_state = lock_handler_unlocked(&state_ctx);
                break;
                
            case STATE_MENU:
                current_state = lock_handler_menu(&state_ctx);
                break;
                
            case STATE_CHANGING_CODE:
                current_state = lock_handler_changing_code(&state_ctx);
                break;
                
            default:
                ESP_LOGE(TAG, "Unknown state: %d", current_state);
                current_state = STATE_LOCKED;
                break;
        }
    }
}
