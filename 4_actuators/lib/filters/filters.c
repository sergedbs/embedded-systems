#include "filters.h"

#include <string.h>

static void sort3(int v[3])
{
    if (v[0] > v[1]) {
        int t = v[0]; v[0] = v[1]; v[1] = t;
    }
    if (v[1] > v[2]) {
        int t = v[1]; v[1] = v[2]; v[2] = t;
    }
    if (v[0] > v[1]) {
        int t = v[0]; v[0] = v[1]; v[1] = t;
    }
}

void filter_median3_reset(filter_median3_t *f)
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

int filter_median3_apply(filter_median3_t *f, int sample)
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

    int tmp[3] = {f->samples[0], f->samples[1], f->samples[2]};
    sort3(tmp);
    return tmp[1];
}

int filter_wma4_apply(filter_wma4_t *f, int sample)
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

    int32_t weighted_sum = 0;
    uint32_t weight_sum = 0;
    for (uint8_t i = 0; i < f->count; ++i) {
        weighted_sum += (int32_t)f->samples[i] * w[i];
        weight_sum += w[i];
    }

    if (weight_sum == 0U) {
        return sample;
    }

    return (int)(weighted_sum / (int32_t)weight_sum);
}
