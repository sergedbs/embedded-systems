#include "oled_display.h"

#include "esp_log.h"

static const char *TAG = "oled_display";

esp_err_t oled_display_init(void)
{
    ESP_LOGI(TAG, "display init stub");
    return ESP_OK;
}

esp_err_t oled_display_render(const system_state_t *snapshot)
{
    (void)snapshot;
    return ESP_OK;
}
