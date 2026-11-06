/**
 * @file    drv_iwdg.c
 * @brief   Watchdog doc lap, chi nap lai khi vong lap chinh that su co tien do.
 */
#include "stm32f4xx.h"
#include "drv_iwdg.h"
#include "drv_tim.h"
#include "config.h"

#define IWDG_KEY_RELOAD   0x0000AAAAu
#define IWDG_KEY_ENABLE   0x0000CCCCu
#define IWDG_KEY_UNLOCK   0x00005555u

static volatile bool s_task_done;
static uint32_t      s_last_ticks;
static bool          s_wdg_reset;

bool drv_iwdg_was_reset_cause(void)
{
    return s_wdg_reset;
}

void drv_iwdg_init(void)
{

    s_wdg_reset = ((RCC->CSR & RCC_CSR_WDGRSTF) != 0u);
    RCC->CSR |= RCC_CSR_RMVF;

    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;

    IWDG->KR  = IWDG_KEY_ENABLE;
    IWDG->KR  = IWDG_KEY_UNLOCK;
    IWDG->PR  = CFG_WDG_PRESCALER_CODE;
    IWDG->RLR = CFG_WDG_RELOAD;
    IWDG->KR  = IWDG_KEY_RELOAD;

    s_task_done  = false;
    s_last_ticks = drv_tim_ticks();
}

void drv_iwdg_note_task_done(void)
{
    s_task_done = true;
}

bool drv_iwdg_service(void)
{
    const uint32_t ticks = drv_tim_ticks();

    if (!s_task_done || (ticks == s_last_ticks)) {
        return false;
    }

    IWDG->KR     = IWDG_KEY_RELOAD;
    s_task_done  = false;
    s_last_ticks = ticks;
    return true;
}
