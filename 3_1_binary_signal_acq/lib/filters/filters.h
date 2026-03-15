#ifndef FILTERS_H
#define FILTERS_H

#include <stdint.h>

#define FILTER_MEDIAN3_SIZE 3
#define FILTER_SMA3_SIZE 3
#define FILTER_WMA4_SIZE 4

typedef struct {
    uint16_t samples[FILTER_MEDIAN3_SIZE];
    uint8_t idx;
    uint8_t count;
} filter_median3_t;

typedef struct {
    uint16_t samples[FILTER_SMA3_SIZE];
    uint8_t idx;
    uint8_t count;
    uint32_t sum;
} filter_sma3_t;

typedef struct {
    uint16_t samples[FILTER_WMA4_SIZE];
    uint8_t count;
} filter_wma4_t;

void filter_median3_reset(filter_median3_t *f);
void filter_sma3_reset(filter_sma3_t *f);
void filter_wma4_reset(filter_wma4_t *f);

uint16_t filter_clamp_u16(uint16_t value, uint16_t min_value, uint16_t max_value);
uint16_t filter_median3_apply(filter_median3_t *f, uint16_t sample);
uint16_t filter_sma3_apply(filter_sma3_t *f, uint16_t sample);
uint16_t filter_wma4_apply(filter_wma4_t *f, uint16_t sample);

#endif // FILTERS_H
