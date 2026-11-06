/**
 * @file    drv_adc.h
 * @brief   ADC1 kenh 0 (PA0) - doc dien ap tu module MQ-2.
 */
#ifndef DRV_ADC_H
#define DRV_ADC_H

#include <stdint.h>
#include <stdbool.h>

bool drv_adc_init(void);

bool drv_adc_calibrated(void);

bool drv_adc_read(uint16_t *out);

#endif
