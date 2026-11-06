/**
 * @file    mq2_sensor.c
 * @brief   Quy doi va loc tin hieu MQ-2, phan muc canh bao. Thuan tinh toan.
 */
#include "mq2_sensor.h"
#include <math.h>

float mq2_adc_to_mv(uint16_t raw)
{
    return ((float)raw / CFG_ADC_MAX) * CFG_ADC_VREF_MV;
}

mq2_status_t mq2_rs_from_adc(uint16_t raw, float *rs_ohm)
{
    if (raw < CFG_ADC_RAW_MIN_VALID) {
        return MQ2_ERR_ADC_LOW;
    }
    if (raw > CFG_ADC_RAW_MAX_VALID) {
        return MQ2_ERR_ADC_HIGH;
    }

    const float vout_mv = mq2_adc_to_mv(raw);

    if (vout_mv >= CFG_MQ2_VC_MV) {
        return MQ2_ERR_ADC_HIGH;
    }

    *rs_ohm = CFG_MQ2_RL_OHM * (CFG_MQ2_VC_MV - vout_mv) / vout_mv;
    return MQ2_OK;
}

float mq2_ppm_from_ratio_pow(float ratio)
{
    if (ratio <= 0.0f) {
        return 0.0f;
    }
    return CFG_MQ2_CURVE_A * powf(ratio, CFG_MQ2_CURVE_B);
}

float mq2_ppm_from_ratio_log(float ratio)
{
    if (ratio <= 0.0f) {
        return 0.0f;
    }
    return powf(10.0f, (log10f(ratio) - CFG_MQ2_LOG_B) / CFG_MQ2_LOG_M);
}

float mq2_ppm_from_ratio(float ratio)
{
    return mq2_ppm_from_ratio_pow(ratio);
}

float mq2_r0_from_rs_air(float rs_air)
{
    return rs_air / CFG_MQ2_CLEAN_AIR_RATIO;
}

static void sort_f(float *a, int n)
{
    for (int i = 1; i < n; i++) {
        const float key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

float mq2_filter_rs(float *rs, int n)
{
    if (n <= 0) {
        return 0.0f;
    }
    if (n == 1) {
        return rs[0];
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        sum += rs[i];
    }
    const float mean = sum / (float)n;

    float var = 0.0f;
    for (int i = 0; i < n; i++) {
        const float d = rs[i] - mean;
        var += d * d;
    }
    const float sd = sqrtf(var / (float)n);
    const float limit = 3.0f * sd;

    int valid = 0;
    for (int i = 0; i < n; i++) {
        if (fabsf(rs[i] - mean) <= limit) {
            rs[valid++] = rs[i];
        }
    }
    if (valid == 0) {
        return mean;
    }

    sort_f(rs, valid);

    const int mid = ((valid % 2) == 1) ? (valid / 2) : ((valid / 2) - 1);
    return rs[mid];
}

mq2_status_t mq2_process(const uint16_t *raw, int n, float *scratch,
                         mq2_result_t *out)
{
    out->rs_ohm  = 0.0f;
    out->ratio   = 0.0f;
    out->ppm     = 0.0f;
    out->raw_mean = 0u;

    if (n <= 0) {
        out->status = MQ2_ERR_ADC_LOW;
        return out->status;
    }

    int valid = 0;
    mq2_status_t first_err = MQ2_OK;
    uint32_t raw_sum = 0u;

    for (int i = 0; i < n; i++) {
        float rs;
        const mq2_status_t st = mq2_rs_from_adc(raw[i], &rs);
        if (st == MQ2_OK) {
            scratch[valid++] = rs;
            raw_sum += raw[i];
        } else if (first_err == MQ2_OK) {
            first_err = st;
        }
    }

    if (valid * 2 <= n) {
        out->status = (first_err == MQ2_OK) ? MQ2_ERR_ADC_LOW : first_err;
        return out->status;
    }

    out->raw_mean = (uint16_t)(raw_sum / (uint32_t)valid);
    out->rs_ohm  = mq2_filter_rs(scratch, valid);
    out->ratio   = out->rs_ohm / CFG_MQ2_R0_OHM;

    if (out->ratio < CFG_MQ2_RATIO_MIN_VALID ||
        out->ratio > CFG_MQ2_RATIO_MAX_VALID) {
        out->status = MQ2_ERR_RATIO;
        return out->status;
    }

    out->ppm    = mq2_ppm_from_ratio(out->ratio);
    out->status = MQ2_OK;
    return out->status;
}

void gas_level_init(gas_level_t *st)
{
    st->level = 0u;
    st->downgrade_count = 0u;
}

uint8_t gas_level_raw(float ppm)
{
    if (ppm >= CFG_GAS_TH3_PPM) {
        return 3u;
    }
    if (ppm >= CFG_GAS_TH2_PPM) {
        return 2u;
    }
    if (ppm >= CFG_GAS_TH1_PPM) {
        return 1u;
    }
    return 0u;
}

static uint8_t gas_level_down(float ppm)
{
    if (ppm >= (CFG_GAS_TH3_PPM - CFG_GAS_HYST3_PPM)) {
        return 3u;
    }
    if (ppm >= (CFG_GAS_TH2_PPM - CFG_GAS_HYST2_PPM)) {
        return 2u;
    }
    if (ppm >= (CFG_GAS_TH1_PPM - CFG_GAS_HYST1_PPM)) {
        return 1u;
    }
    return 0u;
}

void gas_level_invalidate(gas_level_t *st)
{

    st->downgrade_count = 0u;
}

bool meas_is_fresh(bool ever_valid, uint32_t now_ms, uint32_t last_ok_ms,
                   uint32_t max_age_ms)
{
    if (!ever_valid) {
        return false;
    }
    return ((uint32_t)(now_ms - last_ok_ms) <= max_age_ms);
}

uint8_t gas_level_update(gas_level_t *st, float ppm)
{
    const uint8_t up = gas_level_raw(ppm);

    if (up > st->level) {

        st->level = up;
        st->downgrade_count = 0u;
        return st->level;
    }

    const uint8_t down = gas_level_down(ppm);
    if (down < st->level) {
        st->downgrade_count++;
        if (st->downgrade_count >= CFG_GAS_DOWNGRADE_CYCLES) {
            st->level = down;
            st->downgrade_count = 0u;
        }
    } else {
        st->downgrade_count = 0u;
    }

    return st->level;
}
