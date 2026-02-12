#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include "esp_err.h"

/**
 * @brief Initialize buzzer GPIO
 * 
 * Configures buzzer pin as output and turns it off
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t buzzer_init(void);

/**
 * @brief Play a single beep
 * 
 * @param duration_ms Beep duration in milliseconds
 */
void buzzer_beep(uint32_t duration_ms);

/**
 * @brief Play success sound (short beep)
 * 
 * Plays a single 100ms beep to indicate success
 */
void buzzer_success(void);

/**
 * @brief Play error sound (triple beep pattern)
 * 
 * Plays three short beeps with pauses to indicate error
 */
void buzzer_error(void);

/**
 * @brief Turn buzzer on
 */
void buzzer_on(void);

/**
 * @brief Turn buzzer off
 */
void buzzer_off(void);

#endif // BUZZER_H
