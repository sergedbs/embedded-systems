#include "keypad.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "KEYPAD";

// Keypad character mapping
static const char KEYMAP[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// Row and column GPIO arrays
static const gpio_num_t row_pins[KEYPAD_ROWS] = {
    GPIO_KEYPAD_R1,
    GPIO_KEYPAD_R2,
    GPIO_KEYPAD_R3,
    GPIO_KEYPAD_R4
};

static const gpio_num_t col_pins[KEYPAD_COLS] = {
    GPIO_KEYPAD_C1,
    GPIO_KEYPAD_C2,
    GPIO_KEYPAD_C3,
    GPIO_KEYPAD_C4
};

esp_err_t keypad_init(void)
{
    esp_err_t ret;
    
    // Configure row pins as outputs (initially HIGH)
    gpio_config_t row_conf = {
        .pin_bit_mask = (1ULL << GPIO_KEYPAD_R1) |
                        (1ULL << GPIO_KEYPAD_R2) |
                        (1ULL << GPIO_KEYPAD_R3) |
                        (1ULL << GPIO_KEYPAD_R4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ret = gpio_config(&row_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure row pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set all rows HIGH initially
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        gpio_set_level(row_pins[i], 1);
    }
    
    // Configure column pins as inputs with pullup
    gpio_config_t col_conf = {
        .pin_bit_mask = (1ULL << GPIO_KEYPAD_C1) |
                        (1ULL << GPIO_KEYPAD_C2) |
                        (1ULL << GPIO_KEYPAD_C3) |
                        (1ULL << GPIO_KEYPAD_C4),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    ret = gpio_config(&col_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure column pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Keypad initialized successfully");
    return ESP_OK;
}

/**
 * @brief Scan keypad matrix to detect key press
 * 
 * @param row Output: row index where key is pressed
 * @param col Output: column index where key is pressed
 * @return true if a key is pressed, false otherwise
 */
static bool keypad_scan(uint8_t *row, uint8_t *col)
{
    // Scan each row
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        // Set current row LOW, others HIGH
        for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
            gpio_set_level(row_pins[i], (i == r) ? 0 : 1);
        }
        
        // Small delay to let the signal settle
        vTaskDelay(pdMS_TO_TICKS(1));
        
        // Read columns
        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            if (gpio_get_level(col_pins[c]) == 0) {
                // Key pressed (column pulled LOW by row)
                *row = r;
                *col = c;
                
                // Set all rows HIGH before returning
                for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
                    gpio_set_level(row_pins[i], 1);
                }
                
                return true;
            }
        }
    }
    
    // Set all rows HIGH
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
        gpio_set_level(row_pins[i], 1);
    }
    
    return false;
}

char keypad_getkey(void)
{
    uint8_t row, col;
    
    if (keypad_scan(&row, &col)) {
        return KEYMAP[row][col];
    }
    
    return '\0';
}

bool keypad_is_pressed(void)
{
    uint8_t row, col;
    return keypad_scan(&row, &col);
}

char keypad_getkey_blocking(void)
{
    uint8_t row, col;
    char key = '\0';
    
    // Wait for key press
    while (!keypad_scan(&row, &col)) {
        vTaskDelay(pdMS_TO_TICKS(KEYPAD_SCAN_DELAY_MS));
    }
    
    // Debounce: wait and resample
    vTaskDelay(pdMS_TO_TICKS(KEYPAD_DEBOUNCE_MS));
    
    // Verify key is still pressed
    uint8_t verify_row, verify_col;
    if (keypad_scan(&verify_row, &verify_col)) {
        if (verify_row == row && verify_col == col) {
            key = KEYMAP[row][col];
            
            // Wait for key release
            while (keypad_is_pressed()) {
                vTaskDelay(pdMS_TO_TICKS(KEYPAD_SCAN_DELAY_MS));
            }
            
            // Debounce release
            vTaskDelay(pdMS_TO_TICKS(KEYPAD_DEBOUNCE_MS));
            
            ESP_LOGD(TAG, "Key pressed: '%c' (row=%d, col=%d)", key, row, col);
        }
    }
    
    return key;
}
