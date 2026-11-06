/**
 * @file    bsp_systick.h
 * @brief   Nhip thoi gian 1 ms.
 */
#ifndef BSP_SYSTICK_H
#define BSP_SYSTICK_H

#include <stdint.h>
#include <stdbool.h>

void     bsp_systick_init(void);

uint32_t millis(void);

void     delay_ms(uint32_t ms);

bool     elapsed(uint32_t *last, uint32_t period);

#endif
