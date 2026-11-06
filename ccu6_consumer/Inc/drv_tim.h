/**
 * @file    drv_tim.h
 * @brief   TIM3 - nhip 1 ms.
 */
#ifndef DRV_TIM_H
#define DRV_TIM_H

#include <stdint.h>

void     drv_tim_init(void);

uint32_t drv_tim_ticks(void);

void     drv_tim_irq(void);

#endif
