/**
 * @file    bsp_gpio.c
 * @brief   Cau hinh chan cho Producer (CRL/CRH cua F103).
 */
#include "stm32f1xx.h"
#include "bsp_gpio.h"

#define CR_ANALOG     0x0u
#define CR_IN_FLOAT   0x4u
#define CR_IN_PULL    0x8u
#define CR_AF_PP      0xBu
#define CR_AF_OD      0xFu

static void cr_set(volatile uint32_t *cr, uint32_t pin_in_reg, uint32_t cfg)
{
    const uint32_t shift = pin_in_reg * 4u;
    *cr = (*cr & ~(0xFu << shift)) | (cfg << shift);
}

void bsp_gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_AFIOEN;

    cr_set(&GPIOA->CRL, 0u, CR_ANALOG);
    cr_set(&GPIOA->CRL, 2u, CR_AF_PP);
    cr_set(&GPIOA->CRH, 1u, CR_AF_PP);

    GPIOA->BSRR = (1u << 10u);
    cr_set(&GPIOA->CRH, 2u, CR_IN_PULL);
    cr_set(&GPIOB->CRL, 6u, CR_AF_OD);
    cr_set(&GPIOB->CRL, 7u, CR_AF_OD);
}
