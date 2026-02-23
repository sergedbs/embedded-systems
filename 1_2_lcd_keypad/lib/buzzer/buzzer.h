#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize buzzer GPIO
 */
esp_err_t buzzer_init(void);

/**
 * @brief Play a single beep of the given duration
 *
 * @param duration_ms Beep duration in milliseconds
 */
void buzzer_beep(uint32_t duration_ms);

/**
 * @brief Play success sound — single 100ms beep
 */
void buzzer_success(void);

/**
 * @brief Play error sound — triple short beep pattern
 */
void buzzer_error(void);

#endif // BUZZER_H
