#include "app_config.h"
#include "button/button_handler.h"
#include "dht11/dht11_driver.h"
#include "display/oled_display.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_check.h"
#include "esp_log.h"
#include "motion/motion_sensor.h"
#include "system_state.h"
#include "tasks/task_button.h"
#include "tasks/task_dht.h"
#include "tasks/task_display.h"
#include "tasks/task_motion.h"
#include "tasks/task_processing.h"

static const char *TAG = "app_main";
static app_context_t g_app;

static esp_err_t init_core_peripherals(void)
{
    gpio_config_t led_cfg = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_LED_MOTION),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&led_cfg), TAG, "led init failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(PIN_LED_MOTION, 0), TAG, "led off failed");

    const i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_PORT, &i2c_cfg), TAG, "i2c param failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_PORT, i2c_cfg.mode, 0, 0, 0), TAG, "i2c install failed");

    return ESP_OK;
}

void app_main(void)
{
    ESP_LOGI(TAG, "starting iteration 2 architecture skeleton");

    system_state_init(&g_app);
    if (g_app.mutex == NULL) {
        ESP_LOGE(TAG, "system state init failed");
        return;
    }

    if (init_core_peripherals() != ESP_OK) {
        ESP_LOGE(TAG, "core peripheral init failed");
        return;
    }

    (void)dht11_init();
    (void)motion_sensor_init();
    (void)button_handler_init();
    (void)oled_display_init();

    task_dht_start(&g_app);
    task_motion_start(&g_app);
    task_button_start(&g_app);
    task_processing_start(&g_app);
    task_display_start(&g_app);

    ESP_LOGI(TAG, "all tasks started");
}
