/**
 * @file    debounce.h
 * @brief   Chong rung cong tac: chi chot muc khi no da on dinh du lau.
 */
#ifndef DEBOUNCE_H
#define DEBOUNCE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    bool     raw;        /* muc tho o lan tick gan nhat              */
    bool     committed;  /* muc da giu nguyen du lau - dung duoc     */
    bool     reported;   /* muc da bao cho lop tren                  */
    uint16_t stable_ms;  /* so ms muc tho da giu nguyen              */
    uint16_t need_ms;    /* nguong on dinh                           */
} debounce_t;

void debounce_init(debounce_t *d, bool level_now, uint16_t need_ms);

void debounce_tick(debounce_t *d, bool raw);

bool debounce_poll(debounce_t *d, bool *out);

bool debounce_level(const debounce_t *d);

#endif
