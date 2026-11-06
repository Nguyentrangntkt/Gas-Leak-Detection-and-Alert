/**
 * @file    hal_switch.c
 * @brief   Cong tac Start/Stop PC13: doc chan va cap cho bo chong rung.
 */
#include "stm32f4xx.h"
#include "hal_switch.h"
#include "debounce.h"
#include "config.h"

static debounce_t s_db;

bool hal_switch_is_on(void)
{

    return ((GPIOC->IDR & (1u << CFG_SWITCH_PIN)) == 0u);
}

void hal_switch_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    SYSCFG->EXTICR[3] &= ~(0xFu << 4u);
    SYSCFG->EXTICR[3] |=  (2u   << 4u);

    EXTI->IMR  |= (1u << CFG_SWITCH_PIN);
    EXTI->RTSR |= (1u << CFG_SWITCH_PIN);
    EXTI->FTSR |= (1u << CFG_SWITCH_PIN);
    EXTI->PR    = (1u << CFG_SWITCH_PIN);

    NVIC_SetPriority(EXTI15_10_IRQn, 3u);
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    debounce_init(&s_db, hal_switch_is_on(), CFG_DEBOUNCE_MS);
}

void hal_switch_exti_irq(void)
{
    if (EXTI->PR & (1u << CFG_SWITCH_PIN)) {
        EXTI->PR = (1u << CFG_SWITCH_PIN);
    }
}

void hal_switch_tick_1ms(void)
{
    debounce_tick(&s_db, hal_switch_is_on());
}

bool hal_switch_poll(bool *on_out)
{
    return debounce_poll(&s_db, on_out);
}
