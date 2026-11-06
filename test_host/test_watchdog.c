/**
 * @file    test_watchdog.c
 * @brief   Kiem dieu kien nap lai watchdog. Khong mo phong LSI hay reset.
 */
#include "test_util.h"
#include "uart_registers.h"
#include "drv_iwdg.h"
#include "config.h"
RCC_TypeDef host_rcc;
IWDG_TypeDef host_iwdg;
DBGMCU_TypeDef host_dbgmcu;
static uint32_t ticks;
uint32_t drv_tim_ticks(void) { return ticks; }
int main(void)
{
    SUITE("Actual watchdog service: feed requires completed task and advancing TIM3");
    host_rcc.CSR=RCC_CSR_WDGRSTF; ticks=UINT32_MAX;
    drv_iwdg_init();
    CHECK(drv_iwdg_was_reset_cause(),"captures reset cause before clearing flags");
    CHECK_EQ_I(host_iwdg.PR,CFG_WDG_PRESCALER_CODE,"prescaler write");
    CHECK_EQ_I(host_iwdg.RLR,CFG_WDG_RELOAD,"reload write");
    CHECK(host_dbgmcu.APB1FZ & DBGMCU_APB1_FZ_DBG_IWDG_STOP,"debug freeze configured");
    host_iwdg.KR=0u;
    CHECK(!drv_iwdg_service() && host_iwdg.KR==0u,"no completed task -> no feed");
    drv_iwdg_note_task_done();
    CHECK(!drv_iwdg_service() && host_iwdg.KR==0u,"frozen timer -> no feed");
    ticks=0u;
    CHECK(drv_iwdg_service() && host_iwdg.KR==0xAAAAu,"timer rollover counts as progress");
    host_iwdg.KR=0u; ticks=1u;
    CHECK(!drv_iwdg_service() && host_iwdg.KR==0u,"feed consumes task completion");
    drv_iwdg_note_task_done();
    CHECK(drv_iwdg_service(),"next completed task and tick feeds");
    host_rcc.CSR=0u; drv_iwdg_init();
    CHECK(!drv_iwdg_was_reset_cause(),"non-watchdog reset distinguished");
    return test_report("test_watchdog");
}
