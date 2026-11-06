/**
 * @file    bsp_clock.c
 * @brief   Clock F103: HSE 8 MHz + PLL x9 = 72 MHz, tu lui ve HSI neu HSE hong.
 */
#include "stm32f1xx.h"
#include "bsp_clock.h"
#include "config.h"

uint32_t SystemCoreClock = CFG_HSI_HZ;

static uint32_t s_sysclk  = CFG_HSI_HZ;
static uint32_t s_pclk1   = CFG_HSI_HZ;
static uint32_t s_pclk2   = CFG_HSI_HZ;
static uint32_t s_adcclk  = CFG_HSI_HZ / 2u;
static bool     s_degraded = true;
static bool     s_confirmed = true;

void SystemInit(void)
{
}

void SystemCoreClockUpdate(void)
{
    SystemCoreClock = s_sysclk;
}

static bool wait_cr(uint32_t mask, bool want_set)
{
    for (uint32_t i = 0u; i < CFG_CLK_SPIN_LIMIT; i++) {
        if (((RCC->CR & mask) != 0u) == want_set) {
            return true;
        }
    }
    return false;
}

static bool wait_sws(uint32_t sws_value)
{
    for (uint32_t i = 0u; i < CFG_CLK_SPIN_LIMIT; i++) {
        if ((RCC->CFGR & RCC_CFGR_SWS) == sws_value) {
            return true;
        }
    }
    return false;
}

static bool hse_start(void)
{
    RCC->CR |= RCC_CR_HSEON;
    if (wait_cr(RCC_CR_HSERDY, true)) {
        return true;
    }
    RCC->CR &= ~RCC_CR_HSEON;
    return false;
}

static void clock_use_hsi(void)
{

    RCC->CFGR &= ~RCC_CFGR_SW;

    const bool on_hsi = wait_sws(RCC_CFGR_SWS_HSI);

    if (on_hsi) {
        RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 |
                       RCC_CFGR_ADCPRE);
        RCC->CFGR |= RCC_CFGR_ADCPRE_DIV2;

        FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_0;

        RCC->CR &= ~RCC_CR_PLLON;
        (void)wait_cr(RCC_CR_PLLRDY, false);
        RCC->CR &= ~RCC_CR_HSEON;
    }

    s_sysclk   = CFG_HSI_HZ;
    s_pclk1    = CFG_HSI_HZ;
    s_pclk2    = CFG_HSI_HZ;
    s_adcclk   = CFG_HSI_HZ / 2u;
    s_degraded = true;
    s_confirmed = on_hsi;
}

void bsp_clock_init(void)
{

    RCC->CR |= RCC_CR_HSION;
    if (!wait_cr(RCC_CR_HSIRDY, true)) {
        clock_use_hsi();
        SystemCoreClockUpdate();
        return;
    }
    RCC->CFGR &= ~RCC_CFGR_SW;
    (void)wait_sws(RCC_CFGR_SWS_HSI);
    RCC->CR &= ~RCC_CR_PLLON;
    if (!wait_cr(RCC_CR_PLLRDY, false)) {
        clock_use_hsi();
        SystemCoreClockUpdate();
        return;
    }

    if (!hse_start()) {
        clock_use_hsi();
        SystemCoreClockUpdate();
        return;
    }

    FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 |
                   RCC_CFGR_ADCPRE | RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE |
                   RCC_CFGR_PLLMULL);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1
               | RCC_CFGR_PPRE1_DIV2
               | RCC_CFGR_PPRE2_DIV1
               | RCC_CFGR_ADCPRE_DIV6
               | RCC_CFGR_PLLSRC
               | RCC_CFGR_PLLMULL9;

    RCC->CR |= RCC_CR_PLLON;
    if (!wait_cr(RCC_CR_PLLRDY, true)) {
        clock_use_hsi();
        SystemCoreClockUpdate();
        return;
    }

    RCC->CFGR |= RCC_CFGR_SW_PLL;
    if (!wait_sws(RCC_CFGR_SWS_PLL)) {

        clock_use_hsi();
        SystemCoreClockUpdate();
        return;
    }

    s_sysclk   = CFG_TARGET_SYSCLK_HZ;
    s_pclk1    = CFG_TARGET_SYSCLK_HZ / 2u;
    s_pclk2    = CFG_TARGET_SYSCLK_HZ;
    s_adcclk   = CFG_TARGET_SYSCLK_HZ / 6u;
    s_degraded = false;

    SystemCoreClockUpdate();
}

uint32_t bsp_sysclk_hz(void)   { return s_sysclk; }
uint32_t bsp_pclk1_hz(void)    { return s_pclk1;  }
uint32_t bsp_pclk2_hz(void)    { return s_pclk2;  }
uint32_t bsp_adcclk_hz(void)   { return s_adcclk; }
bool     bsp_clock_degraded(void) { return s_degraded; }
bool     bsp_clock_confirmed(void) { return s_confirmed; }
