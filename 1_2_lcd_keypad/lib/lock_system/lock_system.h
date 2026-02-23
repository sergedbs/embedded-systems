/**
 * @file lock_system.h
 * @brief Lock system state machine and UI logic
 * 
 * Handles:
 * - Lock/unlock state machine
 * - First-boot setup wizard
 * - PIN validation and storage (NVS)
 * - User interface flows
 * - Code change functionality
 * 
 * @author ESP32 Lock System
 * @date 2026
 */

#ifndef LOCK_SYSTEM_H
#define LOCK_SYSTEM_H

#include "esp_err.h"

/**
 * @brief Initialize lock system
 * 
 * - Initializes NVS
 * - Loads PIN from NVS if exists
 * - Determines initial state (FIRST_BOOT_SETUP or LOCKED)
 * 
 * Must be called after all peripherals are initialized.
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lock_system_init(void);

/**
 * @brief Run lock system state machine
 * 
 * This is the main loop that never returns.
 * Handles all states:
 * - First boot setup
 * - Locked (PIN entry)
 * - Unlocked (menu)
 * - Code setting/confirmation
 * - Code changing
 * 
 * Call this after lock_system_init().
 */
void lock_system_run(void);

#endif // LOCK_SYSTEM_H
