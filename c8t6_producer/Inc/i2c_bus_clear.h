/**
 * @file    i2c_bus_clear.h
 * @brief   Giai phong bus I2C bi khoa. Thuan tinh toan, kiem thu duoc tren PC.
 */
#ifndef I2C_BUS_CLEAR_H
#define I2C_BUS_CLEAR_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    void *ctx;
    void (*drive)(void *ctx, bool scl_high, bool sda_high);
    uint8_t (*read)(void *ctx);
    uint32_t (*now_ms)(void *ctx);
} i2c_bus_ops_t;

bool i2c_bus_clear(const i2c_bus_ops_t *ops, uint32_t max_ms, uint32_t max_polls);
#endif
