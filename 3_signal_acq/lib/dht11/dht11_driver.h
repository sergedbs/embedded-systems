#ifndef DHT11_DRIVER_H
#define DHT11_DRIVER_H

#include "esp_err.h"

esp_err_t dht11_init(void);
esp_err_t dht11_read(float *temperature_c, float *humidity_pct);

#endif // DHT11_DRIVER_H
