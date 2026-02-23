/**
 * @file lock_handlers.h
 * @brief State handler functions for lock system
 */

#ifndef LOCK_HANDLERS_H
#define LOCK_HANDLERS_H

#include "lock_system.h"

/**
 * @brief State context passed to handlers
 */
typedef struct {
    char *current_pin;        // Current saved PIN
    char *new_pin;            // Buffer for new PIN being set
    lock_state_t *return_state; // State to return to after operation
} lock_state_context_t;

/**
 * @brief Handle first boot setup state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_first_boot_setup(lock_state_context_t *ctx);

/**
 * @brief Handle setting new code state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_setting_code(lock_state_context_t *ctx);

/**
 * @brief Handle confirming code state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_confirming_code(lock_state_context_t *ctx);

/**
 * @brief Handle locked state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_locked(lock_state_context_t *ctx);

/**
 * @brief Handle unlocked state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_unlocked(lock_state_context_t *ctx);

/**
 * @brief Handle menu state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_menu(lock_state_context_t *ctx);

/**
 * @brief Handle changing code state
 * @param ctx State context
 * @return Next state
 */
lock_state_t lock_handler_changing_code(lock_state_context_t *ctx);

#endif // LOCK_HANDLERS_H
