/**
 * @file lock_storage.h
 * @brief NVS storage operations for PIN persistence
 */

#ifndef LOCK_STORAGE_H
#define LOCK_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

/**
 * @brief Load PIN from NVS storage
 * 
 * @param pin_buf Buffer to store the loaded PIN
 * @param buf_size Size of the buffer (must be at least PIN_MAX_LENGTH + 1)
 * @return true if PIN was successfully loaded, false if not found or error
 */
bool lock_storage_load_pin(char *pin_buf, size_t buf_size);

/**
 * @brief Save PIN hash to NVS storage
 *
 * Caller must pass the SHA-256 hex string (64 chars + NUL) produced by
 * hashing the raw PIN.  Validation of PIN policy (length, digits-only)
 * is the caller's responsibility.
 *
 * @param pin_hash 64-character lower-hex SHA-256 digest string
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lock_storage_save_pin(const char *pin_hash);

/**
 * @brief Initialize NVS storage system
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lock_storage_init(void);

#endif // LOCK_STORAGE_H
