#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include "esp_err.h"

esp_err_t dht_sensor_init(void);
esp_err_t dht_sensor_read(float *temperature_c, float *humidity_pct);

#endif // DHT_SENSOR_H
