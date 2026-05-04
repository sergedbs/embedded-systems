#ifndef FILTERS_H
#define FILTERS_H

#include <stdint.h>

#define FILTER_MEDIAN3_SIZE 3
#define FILTER_WMA4_SIZE 4

typedef struct {
    int samples[FILTER_MEDIAN3_SIZE];
    uint8_t idx;
    uint8_t count;
} filter_median3_t;

typedef struct {
    int samples[FILTER_WMA4_SIZE];
    uint8_t count;
} filter_wma4_t;

void filter_median3_reset(filter_median3_t *f);
void filter_wma4_reset(filter_wma4_t *f);

int filter_median3_apply(filter_median3_t *f, int sample);
int filter_wma4_apply(filter_wma4_t *f, int sample);

#endif // FILTERS_H
