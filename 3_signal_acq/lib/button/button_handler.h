#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <stdbool.h>

#include "esp_err.h"

esp_err_t button_handler_init(void);
bool button_handler_poll_pressed(void);

#endif // BUTTON_HANDLER_H
