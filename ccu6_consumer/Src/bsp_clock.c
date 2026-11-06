/**
 * @file    bsp_clock.c
 * @brief   Clock F401: HSE + PLL = 84 MHz, tu lui ve HSI 16 MHz neu HSE hong.
 */
#include "stm32f4xx.h"
#include "bsp_clock.h"
#include "config.h"

uint32_t SystemCoreClock = CFG_HSI_HZ;

static uint32_t s_sysclk    = CFG_HSI_HZ;
static uint32_t s_pclk1     = CFG_HSI_HZ;
static uint32_t s_pclk2     = CFG_HSI_HZ;
static uint32_t s_tim_pclk1 = CFG_HSI_HZ;
static bool     s_degraded  = true;
static bool     s_confirmed = true;

void SystemInit(void)
{

#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10U * 2U) | (3UL << 11U * 2U));
#endif
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
        RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

        FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN |
                     FLASH_ACR_LATENCY_0WS;

        RCC->CR &= ~RCC_CR_PLLON;
        (void)wait_cr(RCC_CR_PLLRDY, false);
        RCC->CR &= ~RCC_CR_HSEON;
    }

    s_confirmed = on_hsi;
    s_sysclk    = CFG_HSI_HZ;
    s_pclk1     = CFG_HSI_HZ;
    s_pclk2     = CFG_HSI_HZ;
    s_tim_pclk1 = CFG_HSI_HZ;
    s_degraded  = true;
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

    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN |
                 FLASH_ACR_LATENCY_2WS;

    const uint32_t pllp_bits = (CFG_PLL_P / 2u) - 1u;

    RCC->PLLCFGR = (CFG_PLL_M << RCC_PLLCFGR_PLLM_Pos)
                 | (CFG_PLL_N << RCC_PLLCFGR_PLLN_Pos)
                 | (pllp_bits << RCC_PLLCFGR_PLLP_Pos)
                 | RCC_PLLCFGR_PLLSRC_HSE
                 | (CFG_PLL_Q << RCC_PLLCFGR_PLLQ_Pos);

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1
               | RCC_CFGR_PPRE1_DIV2
               | RCC_CFGR_PPRE2_DIV1;

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

    s_sysclk    = CFG_TARGET_SYSCLK_HZ;
    s_pclk1     = CFG_TARGET_SYSCLK_HZ / 2u;
    s_pclk2     = CFG_TARGET_SYSCLK_HZ;

    s_tim_pclk1 = CFG_TARGET_SYSCLK_HZ;
    s_degraded  = false;

    SystemCoreClockUpdate();
}

uint32_t bsp_sysclk_hz(void)      { return s_sysclk;    }
uint32_t bsp_pclk1_hz(void)       { return s_pclk1;     }
uint32_t bsp_pclk2_hz(void)       { return s_pclk2;     }
uint32_t bsp_tim_pclk1_hz(void)   { return s_tim_pclk1; }
bool     bsp_clock_degraded(void) { return s_degraded;  }
bool     bsp_clock_confirmed(void) { return s_confirmed; }
