#include "actuators/analog_motor.h"
#include "actuators/binary_actuator.h"
#include "display/lcd_display.h"
#include "esp_err.h"
#include "esp_log.h"
#include "system_state.h"
#include "tasks/task_actuator_output.h"
#include "tasks/task_analog_conditioning.h"
#include "tasks/task_binary_control.h"
#include "tasks/task_command.h"
#include "tasks/task_display.h"

static const char *TAG = "app_main";
static app_context_t g_app;

void app_main(void)
{
    ESP_LOGI(TAG, "starting actuator control application");

    system_state_init(&g_app);
    if (g_app.mutex == NULL) {
        ESP_LOGE(TAG, "system state init failed");
        return;
    }

    ESP_ERROR_CHECK(binary_actuator_init());
    ESP_ERROR_CHECK(analog_motor_init());

    const esp_err_t lcd_err = lcd_display_init();
    if (lcd_err != ESP_OK) {
        ESP_LOGW(TAG, "LCD init failed, continuing with STDIO only: %s", esp_err_to_name(lcd_err));
    }

    if (!task_command_start(&g_app) ||
        !task_binary_control_start(&g_app) ||
        !task_analog_conditioning_start(&g_app) ||
        !task_actuator_output_start(&g_app) ||
        !task_display_start(&g_app)) {
        ESP_LOGE(TAG, "task start failure");
        return;
    }

    ESP_LOGI(TAG, "all actuator tasks started");
}
