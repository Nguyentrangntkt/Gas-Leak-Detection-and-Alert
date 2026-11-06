/**
 * @file    bsp_systick.c
 * @brief   Nhip 1 ms bang SysTick: millis() va delay_ms().
 */
#include "stm32f4xx.h"
#include "bsp_systick.h"
#include "bsp_clock.h"

static volatile uint32_t s_ms = 0u;

void bsp_systick_init(void)
{
    s_ms = 0u;
    SysTick_Config(bsp_sysclk_hz() / 1000u);
    NVIC_SetPriority(SysTick_IRQn, 0u);
}

void SysTick_Handler(void)
{
    s_ms++;
}

uint32_t millis(void)
{
    return s_ms;
}

void delay_ms(uint32_t ms)
{
    const uint32_t start = s_ms;
    while ((uint32_t)(s_ms - start) < ms) {
        __WFI();
    }
}
