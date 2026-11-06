/**
 * @file    bsp_clock.h
 * @brief   Cau hinh clock va tra ve tan so tung bus dang chay.
 */
#ifndef BSP_CLOCK_H
#define BSP_CLOCK_H

#include <stdint.h>
#include <stdbool.h>

void     bsp_clock_init(void);

uint32_t bsp_sysclk_hz(void);
uint32_t bsp_pclk1_hz(void);
uint32_t bsp_pclk2_hz(void);
uint32_t bsp_adcclk_hz(void);

bool     bsp_clock_degraded(void);

bool     bsp_clock_confirmed(void);

#endif
