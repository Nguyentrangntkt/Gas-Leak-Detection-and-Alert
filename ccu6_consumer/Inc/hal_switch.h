/**
 * @file    hal_switch.h
 * @brief   Cong tac Start/Stop PC13 + EXTI13.
 */
#ifndef HAL_SWITCH_H
#define HAL_SWITCH_H

#include <stdbool.h>

void hal_switch_init(void);

bool hal_switch_is_on(void);

void hal_switch_exti_irq(void);

void hal_switch_tick_1ms(void);

bool hal_switch_poll(bool *on_out);

#endif
