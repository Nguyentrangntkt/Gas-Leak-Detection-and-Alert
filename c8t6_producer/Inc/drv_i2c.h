/**
 * @file    drv_i2c.h
 * @brief   I2C1 (PB6/PB7) che do Standard 100 kHz.
 */
#ifndef DRV_I2C_H
#define DRV_I2C_H

#include <stdint.h>
#include <stdbool.h>

void drv_i2c_init(void);

bool drv_i2c_write_byte(uint8_t addr7, uint8_t data);

uint32_t drv_i2c_error_count(void);

#endif
