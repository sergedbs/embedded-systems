#include "display/lcd_display.h"
#include "dht/dht_sensor.h"
#include "esp_err.h"
#include "esp_log.h"
#include "heater/heater_output.h"
#include "system_state.h"
#include "tasks/task_acquisition.h"
#include "tasks/task_command.h"
#include "tasks/task_control.h"
#include "tasks/task_display.h"
#include "tasks/task_output.h"
#include "tasks/task_plotter.h"

static const char *TAG = "app_main";
static app_context_t g_app;

void app_main(void)
{
    ESP_LOGI(TAG, "starting automatic-control baseline");

    system_state_init(&g_app);
    if (g_app.mutex == NULL) {
        ESP_LOGE(TAG, "system state init failed");
        return;
    }

    ESP_ERROR_CHECK(dht_sensor_init());
    ESP_ERROR_CHECK(heater_output_init());

    const esp_err_t lcd_err = lcd_display_init();
    if (lcd_err != ESP_OK) {
        ESP_LOGW(TAG, "LCD init failed, continuing with STDIO only: %s", esp_err_to_name(lcd_err));
    }

    if (!task_command_start(&g_app) ||
        !task_acquisition_start(&g_app) ||
        !task_control_start(&g_app) ||
        !task_output_start(&g_app) ||
        !task_display_start(&g_app) ||
        !task_plotter_start(&g_app)) {
        ESP_LOGE(TAG, "task start failure");
        return;
    }

    ESP_LOGI(TAG, "all automatic-control tasks started");
}
