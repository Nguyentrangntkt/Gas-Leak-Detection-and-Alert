/**
 * @file    drv_iwdg.h
 * @brief   Watchdog doc lap (IWDG) cho Consumer.
 */
#ifndef DRV_IWDG_H
#define DRV_IWDG_H

#include <stdint.h>
#include <stdbool.h>

void drv_iwdg_init(void);

void drv_iwdg_note_task_done(void);

bool drv_iwdg_service(void);

bool drv_iwdg_was_reset_cause(void);

#endif
