/**
 * @file    i2c_bus_clear.c
 * @brief   Giai phong bus I2C bi slave giu: 9 xung SCL + chuoi theo errata.
 */
#include "i2c_bus_clear.h"
typedef struct {
    const i2c_bus_ops_t *ops;
    uint32_t start, max_ms, polls;
} budget_t;
static bool within(budget_t *b)
{
    if (b->polls == 0u) { return false; }
    b->polls--;
    return (uint32_t)(b->ops->now_ms(b->ops->ctx) - b->start) < b->max_ms;
}
static bool lines(budget_t *b, uint8_t mask, uint8_t value)
{
    while (within(b)) {
        if ((b->ops->read(b->ops->ctx) & mask) == value) { return true; }
    }
    return false;
}

static bool hold(budget_t *b)
{
    const uint32_t start = b->ops->now_ms(b->ops->ctx);
    while ((uint32_t)(b->ops->now_ms(b->ops->ctx) - start) < 1u) {
        if (!within(b)) { return false; }
    }
    return within(b);
}
bool i2c_bus_clear(const i2c_bus_ops_t *ops, uint32_t max_ms, uint32_t max_polls)
{
    budget_t b = { ops, ops->now_ms(ops->ctx), max_ms, max_polls };
    bool ok = false;
    ops->drive(ops->ctx, true, true);
    if (!lines(&b, 1u, 1u)) { goto done; }

    for (unsigned n = 0u; n < 9u && (ops->read(ops->ctx) & 2u) == 0u; n++) {
        ops->drive(ops->ctx, false, true);
        if (!lines(&b, 1u, 0u) || !hold(&b)) { goto done; }
        ops->drive(ops->ctx, true, true);
        if (!lines(&b, 1u, 1u) || !hold(&b)) { goto done; }
    }
    if (!lines(&b, 3u, 3u)) { goto done; }

    ops->drive(ops->ctx, true, false);
    if (!lines(&b, 3u, 1u) || !hold(&b)) { goto done; }
    ops->drive(ops->ctx, false, false);
    if (!lines(&b, 3u, 0u) || !hold(&b)) { goto done; }
    ops->drive(ops->ctx, true, false);
    if (!lines(&b, 3u, 1u) || !hold(&b)) { goto done; }
    ops->drive(ops->ctx, true, true);
    ok = lines(&b, 3u, 3u) && hold(&b);
done:
    ops->drive(ops->ctx, true, true);
    return ok;
}
