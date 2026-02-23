/**
 * @file lock_system.c
 * @brief Lock system state machine coordination
 */

#include "lock_system.h"
#include "lock_storage.h"
#include "lock_handlers.h"
#include "config_pins.h"
#include "config_app.h"
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
static uint8_t failed_attempts = 0;
static bool was_auto_locked = false;  // Flag for auto-lock timer

// Auto-lock timer
static TimerHandle_t autolock_timer = NULL;
static bool autolock_enabled = false;

// Backlight auto-off timer
static TimerHandle_t backlight_timer = NULL;
static bool backlight_timer_enabled = false;

// State context for handlers
static lock_state_context_t state_ctx = {
    .current_pin = current_pin,
    .new_pin = new_pin,
    .return_state = &return_state,
    .failed_attempts = &failed_attempts,
    .current_state = &current_state,
    .was_auto_locked = &was_auto_locked
};

// ============================================================================
// Auto-lock Timer
// ============================================================================

/**
 * @brief Auto-lock timer callback - transitions to LOCKED state
 * NON-BLOCKING: Timer callbacks must not have delays or blocking LCD operations
 */
static void autolock_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Auto-lock timer expired - locking system");
    
    // Set flag so LOCKED handler can display "AUTO-LOCKED"
    was_auto_locked = true;
    
    // Non-blocking: just transition state, let state machine handle display
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
// Backlight Auto-Off Timer
// ============================================================================

/**
 * @brief Backlight timer callback - turns off backlight after 60 seconds
 */
static void backlight_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI(TAG, "Backlight timer expired - turning off backlight");
    lcd_backlight(false);
    backlight_timer_enabled = false;
}

void lock_system_start_backlight_timer(void)
{
    // Always turn on backlight when timer starts
    lcd_backlight(true);
    
    if (backlight_timer == NULL) {
        // Create timer on first use (60 seconds)
        backlight_timer = xTimerCreate(
            "backlight",
            pdMS_TO_TICKS(BACKLIGHT_TIMEOUT_SEC * 1000),
            pdFALSE,  // One-shot timer
            NULL,
            backlight_timer_callback
        );
        
        if (backlight_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create backlight timer");
            return;
        }
    }
    
    // Start/restart the timer
    if (xTimerStart(backlight_timer, 0) == pdPASS) {
        backlight_timer_enabled = true;
        ESP_LOGD(TAG, "Backlight timer started (60 seconds)");
    } else {
        ESP_LOGE(TAG, "Failed to start backlight timer");
    }
}

void lock_system_reset_backlight_timer(void)
{
    // Always turn on backlight when activity detected
    lcd_backlight(true);
    
    if (backlight_timer == NULL) {
        // Timer not created yet - create and start it
        lock_system_start_backlight_timer();
        return;
    }
    
    // For one-shot timers, check if timer is active
    if (xTimerIsTimerActive(backlight_timer)) {
        // Timer is running - reset it (extends the 60s period)
        if (xTimerReset(backlight_timer, 0) == pdPASS) {
            backlight_timer_enabled = true;  // Ensure flag stays true
            ESP_LOGD(TAG, "Backlight timer reset");
        } else {
            ESP_LOGW(TAG, "Failed to reset backlight timer");
        }
    } else {
        // Timer has expired or stopped - restart it
        if (xTimerStart(backlight_timer, 0) == pdPASS) {
            backlight_timer_enabled = true;
            ESP_LOGD(TAG, "Backlight timer restarted");
        } else {
            ESP_LOGW(TAG, "Failed to restart backlight timer");
        }
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
    
    // Start backlight timer
    lock_system_start_backlight_timer();
    
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
                
            case STATE_LOCKOUT:
                current_state = lock_handler_lockout(&state_ctx);
                break;
                
            default:
                ESP_LOGE(TAG, "Unknown state: %d", current_state);
                current_state = STATE_LOCKED;
                break;
        }
    }
}
