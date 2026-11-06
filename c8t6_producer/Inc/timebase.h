/**
 * @file    timebase.h
 * @brief   So hoc moc thoi gian, chiu duoc moc tran 32 bit. Thuan tinh toan.
 */
#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>
#include <stdbool.h>

static inline bool time_elapsed(uint32_t now, uint32_t *last, uint32_t period)
{
    if ((uint32_t)(now - *last) < period) {
        return false;
    }
    *last = now;
    return true;
}

#endif
