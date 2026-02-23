/**
 * @file lock_storage.c
 * @brief NVS storage operations for PIN persistence
 */

#include "lock_storage.h"
#include "config_pins.h"
#include "config_app.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "LOCK_STORAGE";

// NVS namespace and key
#define NVS_NAMESPACE "storage"
#define NVS_PIN_KEY   "user_pin"

esp_err_t lock_storage_init(void)
{
    ESP_LOGI(TAG, "Initializing NVS storage...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "NVS storage initialized successfully");
    return ESP_OK;
}

bool lock_storage_load_pin(char *pin_buf, size_t buf_size)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for reading: %s", esp_err_to_name(err));
        return false;
    }
    
    // Try to read PIN
    size_t required_size = buf_size;
    err = nvs_get_str(nvs_handle, NVS_PIN_KEY, pin_buf, &required_size);
    
    nvs_close(nvs_handle);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "PIN loaded from NVS (length: %d)", strlen(pin_buf));
        return true;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No PIN found in NVS - first boot");
        return false;
    } else {
        ESP_LOGE(TAG, "Error reading PIN from NVS: %s", esp_err_to_name(err));
        return false;
    }
}

esp_err_t lock_storage_save_pin(const char *pin)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    
    // Validate PIN length
    size_t pin_len = strlen(pin);
    if (pin_len < PIN_MIN_LENGTH || pin_len > PIN_MAX_LENGTH) {
        ESP_LOGE(TAG, "Invalid PIN length: %d (must be %d-%d)", 
                 pin_len, PIN_MIN_LENGTH, PIN_MAX_LENGTH);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Open NVS
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for writing: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write PIN
    err = nvs_set_str(nvs_handle, NVS_PIN_KEY, pin);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write PIN to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }
    
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "PIN saved to NVS successfully");
    return ESP_OK;
}
