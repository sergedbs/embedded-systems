#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include "esp_err.h"
#include "system_state.h"

esp_err_t oled_display_init(void);
esp_err_t oled_display_render(const system_state_t *snapshot);

#endif // OLED_DISPLAY_H
