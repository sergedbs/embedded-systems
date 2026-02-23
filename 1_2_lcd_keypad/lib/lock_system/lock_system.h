/**
 * @file lock_system.h
 * @brief Lock system state machine and public API
 */

#ifndef LOCK_SYSTEM_H
#define LOCK_SYSTEM_H

#include "esp_err.h"

/**
 * @brief Lock system states
 */
typedef enum {
    STATE_FIRST_BOOT_SETUP,    // Setup wizard - set initial PIN
    STATE_SETTING_CODE,        // User entering new PIN
    STATE_CONFIRMING_CODE,     // User confirming new PIN
    STATE_LOCKED,              // System locked - waiting for PIN
    STATE_UNLOCKED,            // System unlocked - show success
    STATE_MENU,                // Display menu options
    STATE_CHANGING_CODE        // Validate old PIN before changing
} lock_state_t;

/**
 * @brief Initialize lock system
 * 
 * Initializes NVS, loads saved PIN if exists, sets initial state
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lock_system_init(void);

/**
 * @brief Run lock system state machine
 * 
 * Main loop - never returns
 */
void lock_system_run(void);

#endif // LOCK_SYSTEM_H
