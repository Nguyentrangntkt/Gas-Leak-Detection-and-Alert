/**
 * @file    drv_tim.c
 * @brief   TIM3 sinh nhip 1 ms cho mau nhay LED va bo chong rung.
 */
#include "stm32f4xx.h"
#include "drv_tim.h"
#include "bsp_clock.h"
#include "hal_switch.h"
#include "config.h"

static volatile uint32_t s_ticks = 0u;

void drv_tim_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    const uint32_t tim_clk = bsp_tim_pclk1_hz();
    TIM3->PSC  = (uint16_t)((tim_clk / 10000u) - 1u);
    TIM3->ARR  = (uint16_t)((10000u / CFG_TIM_TICK_HZ) - 1u);
    TIM3->CNT  = 0u;
    TIM3->EGR  = TIM_EGR_UG;
    TIM3->SR   = 0u;
    TIM3->DIER = TIM_DIER_UIE;
    TIM3->CR1  = TIM_CR1_CEN;

    NVIC_SetPriority(TIM3_IRQn, 2u);
    NVIC_EnableIRQ(TIM3_IRQn);
}

uint32_t drv_tim_ticks(void)
{
    return s_ticks;
}

void drv_tim_irq(void)
{
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR = (uint16_t)~TIM_SR_UIF;
        s_ticks++;
        hal_switch_tick_1ms();
    }
}
