/**
 * @file    mq2_sensor.h
 * @brief   Quy doi MQ-2: ADC -> Rs -> ppm, loc nhieu, phan muc co hysteresis.
 */
#ifndef MQ2_SENSOR_H
#define MQ2_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

typedef enum {
    MQ2_OK = 0,
    MQ2_ERR_ADC_LOW,
    MQ2_ERR_ADC_HIGH,
    MQ2_ERR_RATIO
} mq2_status_t;

typedef struct {
    float        rs_ohm;
    float        ratio;
    float        ppm;
    uint16_t     raw_mean;
    mq2_status_t status;
} mq2_result_t;

float mq2_adc_to_mv(uint16_t raw);

mq2_status_t mq2_rs_from_adc(uint16_t raw, float *rs_ohm);

float mq2_ppm_from_ratio_pow(float ratio);

float mq2_ppm_from_ratio_log(float ratio);

float mq2_ppm_from_ratio(float ratio);

float mq2_r0_from_rs_air(float rs_air);

float mq2_filter_rs(float *rs, int n);

mq2_status_t mq2_process(const uint16_t *raw, int n, float *scratch,
                         mq2_result_t *out);

typedef struct {
    uint8_t level;
    uint8_t downgrade_count;
} gas_level_t;

void    gas_level_init(gas_level_t *st);

uint8_t gas_level_raw(float ppm);

uint8_t gas_level_update(gas_level_t *st, float ppm);

void    gas_level_invalidate(gas_level_t *st);

bool    meas_is_fresh(bool ever_valid, uint32_t now_ms, uint32_t last_ok_ms,
                      uint32_t max_age_ms);

#endif
