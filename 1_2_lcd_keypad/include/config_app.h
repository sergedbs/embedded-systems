/**
 * @file config_app.h
 * @brief Application-level configuration constants
 *
 * Hardware pin assignments and peripheral setup live in config_pins.h.
 * This file contains only application behaviour constants: timeouts,
 * PIN policy, and UI timing.
 */

#ifndef CONFIG_APP_H
#define CONFIG_APP_H

// ============================================================================
// Security / PIN Policy
// ============================================================================
#define PIN_MIN_LENGTH          4       // Minimum PIN length (digits)
#define PIN_MAX_LENGTH          8       // Maximum PIN length (digits)
#define MAX_FAILED_ATTEMPTS     3       // Failed attempts before lockout

// ============================================================================
// Timers
// ============================================================================
#define AUTOLOCK_TIMEOUT_SEC    30      // Idle time in MENU before auto-lock
#define BACKLIGHT_TIMEOUT_SEC   60      // Idle time before backlight off
#define LOCKOUT_DURATION_SEC    30      // How long the lockout lasts

// ============================================================================
// UI / Input
// ============================================================================
#define CHAR_REVEAL_MS          300     // Last-char reveal window (non-blocking)

#endif // CONFIG_APP_H
