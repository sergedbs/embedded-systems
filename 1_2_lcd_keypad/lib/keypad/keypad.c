#include "keypad.h"
#include "config_pins.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "KEYPAD";

static const char KEYMAP[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

static const gpio_num_t row_pins[KEYPAD_ROWS] = {
    GPIO_KEYPAD_R1, GPIO_KEYPAD_R2, GPIO_KEYPAD_R3, GPIO_KEYPAD_R4
};

static const gpio_num_t col_pins[KEYPAD_COLS] = {
    GPIO_KEYPAD_C1, GPIO_KEYPAD_C2, GPIO_KEYPAD_C3, GPIO_KEYPAD_C4
};

/** Set all row pins HIGH (idle state) */
static void keypad_rows_idle(void)
{
    for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
        gpio_set_level(row_pins[i], 1);
    }
}

esp_err_t keypad_init(void)
{
    gpio_config_t row_conf = {
        .pin_bit_mask = (1ULL << GPIO_KEYPAD_R1) | (1ULL << GPIO_KEYPAD_R2) |
                        (1ULL << GPIO_KEYPAD_R3) | (1ULL << GPIO_KEYPAD_R4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&row_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure row pins: %s", esp_err_to_name(ret));
        return ret;
    }

    keypad_rows_idle();

    gpio_config_t col_conf = {
        .pin_bit_mask = (1ULL << GPIO_KEYPAD_C1) | (1ULL << GPIO_KEYPAD_C2) |
                        (1ULL << GPIO_KEYPAD_C3) | (1ULL << GPIO_KEYPAD_C4),
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

    ESP_LOGI(TAG, "Keypad initialized");
    return ESP_OK;
}

/**
 * @brief Scan matrix for a pressed key
 *
 * @param row  Output row index
 * @param col  Output column index
 * @return true if a key is pressed
 */
static bool keypad_scan(uint8_t *row, uint8_t *col)
{
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        // Pull current row LOW
        for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
            gpio_set_level(row_pins[i], (i == r) ? 0 : 1);
        }
        vTaskDelay(pdMS_TO_TICKS(1));

        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            if (gpio_get_level(col_pins[c]) == 0) {
                *row = r;
                *col = c;
                keypad_rows_idle();
                return true;
            }
        }
    }

    keypad_rows_idle();
    return false;
}

static bool keypad_is_pressed(void)
{
    uint8_t row, col;
    return keypad_scan(&row, &col);
}

char keypad_getkey(void)
{
    uint8_t row, col;
    return keypad_scan(&row, &col) ? KEYMAP[row][col] : '\0';
}

char keypad_getkey_blocking(void)
{
    uint8_t row, col;

    // Wait for press
    while (!keypad_scan(&row, &col)) {
        vTaskDelay(pdMS_TO_TICKS(KEYPAD_SCAN_DELAY_MS));
    }

    // Debounce: wait and verify same key still held
    vTaskDelay(pdMS_TO_TICKS(KEYPAD_DEBOUNCE_MS));

    uint8_t vrow, vcol;
    if (!keypad_scan(&vrow, &vcol) || vrow != row || vcol != col) {
        return '\0';  // Bounce or different key — discard
    }

    char key = KEYMAP[row][col];

    // Wait for release + debounce
    while (keypad_is_pressed()) {
        vTaskDelay(pdMS_TO_TICKS(KEYPAD_SCAN_DELAY_MS));
    }
    vTaskDelay(pdMS_TO_TICKS(KEYPAD_DEBOUNCE_MS));

    ESP_LOGD(TAG, "Key: '%c'", key);
    return key;
}
