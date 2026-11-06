/**
 * @file    stm32f4xx_it.c
 * @brief   Bang ngat cua F401: chuyen tiep sang cac driver tuong ung.
 */
#include "stm32f4xx.h"
#include "drv_uart.h"
#include "drv_tim.h"
#include "hal_switch.h"

void USART2_IRQHandler(void)
{
    drv_uart_irq();
}

void TIM3_IRQHandler(void)
{
    drv_tim_irq();
}

void EXTI15_10_IRQHandler(void)
{
    hal_switch_exti_irq();
}

void NMI_Handler(void)        { for (;;) { } }
void HardFault_Handler(void)  { for (;;) { } }
void MemManage_Handler(void)  { for (;;) { } }
void BusFault_Handler(void)   { for (;;) { } }
void UsageFault_Handler(void) { for (;;) { } }

void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }
