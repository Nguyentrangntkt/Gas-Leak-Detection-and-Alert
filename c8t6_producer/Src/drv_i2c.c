/**
 * @file    drv_i2c.c
 * @brief   I2C1 100 kHz co timeout va duong phuc hoi khi bus bi khoa.
 */
#include "stm32f1xx.h"
#include "drv_i2c.h"
#include "bsp_clock.h"
#include "config.h"
#include "bsp_systick.h"
#include "i2c_bus_clear.h"

static volatile uint32_t s_errors = 0u;

static bool wait_flag(volatile uint32_t *reg, uint32_t mask, bool set)
{
    for (uint32_t i = 0u; i < CFG_I2C_SPIN_LIMIT; i++) {
        const bool now = ((*reg & mask) != 0u);
        if (now == set) {
            return true;
        }
    }
    return false;
}

static void i2c_configure(void)
{
    const uint32_t pclk1     = bsp_pclk1_hz();
    const uint32_t pclk1_mhz = pclk1 / 1000000u;

    I2C1->CR2   = pclk1_mhz;

    I2C1->CCR   = pclk1 / (2u * 100000u);
    I2C1->TRISE = pclk1_mhz + 1u;
    I2C1->CR1  |= I2C_CR1_PE;
}

void drv_i2c_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0u;

    i2c_configure();
}

static void bus_drive(void *ctx, bool scl_high, bool sda_high)
{
    (void)ctx;
    GPIOB->BSRR = (scl_high ? (1u << 6) : (1u << 22))
                | (sda_high ? (1u << 7) : (1u << 23));
}
static uint8_t bus_read(void *ctx)
{
    (void)ctx;
    return (uint8_t)((GPIOB->IDR >> 6) & 3u);
}
static uint32_t bus_now(void *ctx) { (void)ctx; return millis(); }

static void i2c_recover(void)
{

    I2C1->SR1 = ~(uint32_t)(I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO |
                            I2C_SR1_OVR);

    if (!wait_flag(&I2C1->SR2, I2C_SR2_BUSY, false)) {
        const i2c_bus_ops_t ops = { 0, bus_drive, bus_read, bus_now };
        I2C1->CR1 &= ~I2C_CR1_PE;
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        bus_drive(0, true, true);
        GPIOB->CRL = (GPIOB->CRL & ~0xFF000000u) | 0x77000000u;

        (void)i2c_bus_clear(&ops, CFG_I2C_RECOVERY_MS, CFG_I2C_RECOVERY_POLLS);
        GPIOB->CRL = (GPIOB->CRL & ~0xFF000000u) | 0xFF000000u;
        I2C1->CR1 = I2C_CR1_SWRST;
        I2C1->CR1 = 0u;
        i2c_configure();
    }
}

bool drv_i2c_write_byte(uint8_t addr7, uint8_t data)
{

    if (!wait_flag(&I2C1->SR2, I2C_SR2_BUSY, false)) {
        goto fail;
    }

    I2C1->CR1 |= I2C_CR1_START;
    if (!wait_flag(&I2C1->SR1, I2C_SR1_SB, true)) {
        goto fail;
    }

    I2C1->DR = (uint8_t)(addr7 << 1);
    for (uint32_t i = 0u; i < CFG_I2C_SPIN_LIMIT; i++) {
        if (I2C1->SR1 & I2C_SR1_ADDR) {
            (void)I2C1->SR1;
            (void)I2C1->SR2;
            goto addressed;
        }
        if (I2C1->SR1 & I2C_SR1_AF) {
            goto fail;
        }
    }
    goto fail;

addressed:
    if (!wait_flag(&I2C1->SR1, I2C_SR1_TXE, true)) {
        goto fail;
    }
    I2C1->DR = data;
    if (!wait_flag(&I2C1->SR1, I2C_SR1_BTF, true)) {
        goto fail;
    }

    I2C1->CR1 |= I2C_CR1_STOP;
    return true;

fail:
    I2C1->CR1 |= I2C_CR1_STOP;
    i2c_recover();
    s_errors++;
    return false;
}

uint32_t drv_i2c_error_count(void)
{
    return s_errors;
}
