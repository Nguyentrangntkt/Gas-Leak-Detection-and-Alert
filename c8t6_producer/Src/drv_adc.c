/**
 * @file    drv_adc.c
 * @brief   ADC1 kenh 0 doc MQ-2. Moi vong cho deu co han muc.
 */
#include "stm32f1xx.h"
#include "drv_adc.h"
#include "bsp_systick.h"
#include "config.h"

static bool s_cal_ok;

static bool wait_cr2_clear(uint32_t mask)
{
    for (uint32_t i = 0u; i < CFG_ADC_SPIN_LIMIT; i++) {
        if ((ADC1->CR2 & mask) == 0u) {
            return true;
        }
    }
    return false;
}

bool drv_adc_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    ADC1->CR1 = 0u;
    ADC1->CR2 = 0u;

    ADC1->SMPR2 = (ADC1->SMPR2 & ~ADC_SMPR2_SMP0) | ADC_SMPR2_SMP0;

    ADC1->SQR1 = 0u;
    ADC1->SQR3 = CFG_ADC_CHANNEL;

    ADC1->CR2 |= ADC_CR2_ADON;
    delay_ms(1u);

    s_cal_ok = false;
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    if (wait_cr2_clear(ADC_CR2_RSTCAL)) {
        ADC1->CR2 |= ADC_CR2_CAL;
        s_cal_ok = wait_cr2_clear(ADC_CR2_CAL);
    }

    return s_cal_ok;
}

bool drv_adc_calibrated(void)
{
    return s_cal_ok;
}

bool drv_adc_read(uint16_t *out)
{
    ADC1->SQR3 = CFG_ADC_CHANNEL;

    ADC1->CR2 |= ADC_CR2_EXTSEL;
    ADC1->CR2 |= ADC_CR2_EXTTRIG;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    for (uint32_t i = 0u; i < CFG_ADC_SPIN_LIMIT; i++) {
        if (ADC1->SR & ADC_SR_EOC) {
            *out = (uint16_t)(ADC1->DR & 0x0FFFu);
            return true;
        }
    }

    ADC1->SR &= ~(uint32_t)(ADC_SR_EOC | ADC_SR_STRT);
    return false;
}
