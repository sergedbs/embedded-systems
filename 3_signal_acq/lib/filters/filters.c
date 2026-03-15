#include "filters.h"

#include <string.h>

static void sort3(uint16_t v[3])
{
    if (v[0] > v[1]) {
        uint16_t t = v[0]; v[0] = v[1]; v[1] = t;
    }
    if (v[1] > v[2]) {
        uint16_t t = v[1]; v[1] = v[2]; v[2] = t;
    }
    if (v[0] > v[1]) {
        uint16_t t = v[0]; v[0] = v[1]; v[1] = t;
    }
}

void filter_median3_reset(filter_median3_t *f)
{
    if (f != NULL) {
        memset(f, 0, sizeof(*f));
    }
}

void filter_sma3_reset(filter_sma3_t *f)
{
    if (f != NULL) {
        memset(f, 0, sizeof(*f));
    }
}

void filter_wma4_reset(filter_wma4_t *f)
{
    if (f != NULL) {
        memset(f, 0, sizeof(*f));
    }
}

uint16_t filter_clamp_u16(uint16_t value, uint16_t min_value, uint16_t max_value)
{
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

uint16_t filter_median3_apply(filter_median3_t *f, uint16_t sample)
{
    if (f == NULL) {
        return sample;
    }

    f->samples[f->idx] = sample;
    f->idx = (uint8_t)((f->idx + 1U) % FILTER_MEDIAN3_SIZE);
    if (f->count < FILTER_MEDIAN3_SIZE) {
        f->count++;
    }

    if (f->count < FILTER_MEDIAN3_SIZE) {
        return sample;
    }

    uint16_t tmp[3] = {f->samples[0], f->samples[1], f->samples[2]};
    sort3(tmp);
    return tmp[1];
}

uint16_t filter_sma3_apply(filter_sma3_t *f, uint16_t sample)
{
    if (f == NULL) {
        return sample;
    }

    if (f->count < FILTER_SMA3_SIZE) {
        f->samples[f->idx] = sample;
        f->sum += sample;
        f->count++;
    } else {
        f->sum -= f->samples[f->idx];
        f->samples[f->idx] = sample;
        f->sum += sample;
    }

    f->idx = (uint8_t)((f->idx + 1U) % FILTER_SMA3_SIZE);

    const uint8_t denom = (f->count == 0U) ? 1U : f->count;
    return (uint16_t)(f->sum / denom);
}

uint16_t filter_wma4_apply(filter_wma4_t *f, uint16_t sample)
{
    if (f == NULL) {
        return sample;
    }

    f->samples[3] = f->samples[2];
    f->samples[2] = f->samples[1];
    f->samples[1] = f->samples[0];
    f->samples[0] = sample;

    if (f->count < FILTER_WMA4_SIZE) {
        f->count++;
    }

    // Weights for newest->oldest: 50, 25, 15, 10
    static const uint8_t w[FILTER_WMA4_SIZE] = {50, 25, 15, 10};

    uint32_t weighted_sum = 0;
    uint32_t weight_sum = 0;
    for (uint8_t i = 0; i < f->count; ++i) {
        weighted_sum += (uint32_t)f->samples[i] * w[i];
        weight_sum += w[i];
    }

    if (weight_sum == 0U) {
        return sample;
    }

    return (uint16_t)(weighted_sum / weight_sum);
}
