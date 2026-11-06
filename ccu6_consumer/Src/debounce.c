/**
 * @file    debounce.c
 * @brief   Chong rung cong tac. Thuan tinh toan, kiem thu duoc tren PC.
 */
#include "debounce.h"

void debounce_init(debounce_t *d, bool level_now, uint16_t need_ms)
{
    d->raw       = level_now;
    d->committed = level_now;
    d->reported  = level_now;
    d->stable_ms = need_ms;
    d->need_ms   = need_ms;
}

void debounce_tick(debounce_t *d, bool raw)
{
    if (raw != d->raw) {

        d->raw       = raw;
        d->stable_ms = 1u;

        if (d->need_ms <= 1u) {
            d->committed = raw;
        }
        return;
    }

    if (d->stable_ms < d->need_ms) {
        d->stable_ms++;
        if (d->stable_ms >= d->need_ms) {

            d->committed = d->raw;
        }
    }
}

bool debounce_poll(debounce_t *d, bool *out)
{
    const bool level = d->committed;

    if (level == d->reported) {
        return false;
    }
    d->reported = level;
    *out        = level;
    return true;
}

bool debounce_level(const debounce_t *d)
{
    return d->committed;
}
