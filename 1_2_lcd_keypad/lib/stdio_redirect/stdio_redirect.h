/**
 * @file stdio_redirect.h
 * @brief VFS-based STDIO redirection: stdout → LCD, stdin → GPIO keypad.
 *
 * After stdio_redirect_init(), bare printf() prints to the LCD and bare
 * scanf() reads from the keypad.  ESP_LOGI/ESP_LOGE continue writing to
 * UART via esp_log_set_vprintf().
 *
 * Typical usage per input operation:
 *   set_input_mode(INPUT_MODE_MASKED, true);
 *   printf("PIN: ");
 *   int n = scanf("%8s", buf);
 *   if (stdin_was_cancelled()) { ... }
 *
 * @author ESP32 Lock System
 * @date 2026
 */

#ifndef STDIO_REDIRECT_H
#define STDIO_REDIRECT_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Input masking modes for stdin (keypad) reads.
 */
typedef enum {
    INPUT_MODE_NORMAL,       ///< Display characters as typed
    INPUT_MODE_MASKED,       ///< Display asterisks for all input
    INPUT_MODE_REVEAL_LAST   ///< Show last char briefly, then mask it
} input_mode_t;

/**
 * @brief Register VFS drivers and redirect stdout → LCD, stdin → keypad.
 *
 * Must be called after lcd_init() and keypad_init().
 * ESP log output is preserved on UART.
 *
 * @return ESP_OK on success, ESP_FAIL on VFS/timer error.
 */
esp_err_t stdio_redirect_init(void);

/**
 * @brief Configure the input mode and digit-only filter for the next read.
 *
 * Also clears any previous EOF/cancel state on stdin, so this must be
 * called once before each scanf() that follows a cancel.
 *
 * @param mode        Masking behaviour.
 * @param digits_only If true, non-digit keys (except '#' and '*') are ignored.
 */
void set_input_mode(input_mode_t mode, bool digits_only);

/**
 * @brief Register a callback invoked on every keypress during stdin reads.
 *
 * Typically used to reset activity timers without coupling the input
 * layer to the application layer.
 *
 * @param cb Callback function pointer, or NULL to clear.
 */
void set_keypress_cb(void (*cb)(void));

/**
 * @brief Returns true if the last stdin read was cancelled by the 'C' key.
 *
 * Valid after a scanf() that returned EOF (-1).
 * Automatically cleared by the next call to set_input_mode().
 *
 * @return true if cancelled, false otherwise.
 */
bool stdin_was_cancelled(void);

#endif // STDIO_REDIRECT_H
