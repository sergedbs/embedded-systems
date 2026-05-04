#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <stdbool.h>

#include "esp_err.h"
#include "system_state.h"

esp_err_t lcd_display_init(void);
esp_err_t lcd_display_render(const system_state_t *snapshot);
bool lcd_display_is_ready(void);

#endif // LCD_DISPLAY_H
