/**
 * @file    stm32f1xx_it.c
 * @brief   Bang ngat cua F103: chuyen tiep sang cac driver tuong ung.
 */
#include "stm32f1xx.h"
#include "drv_uart.h"

void USART1_IRQHandler(void)
{
    drv_uart_irq(UART_LINK);
}

void USART2_IRQHandler(void)
{
    drv_uart_irq(UART_ESP32);
}

void NMI_Handler(void)        { for (;;) { } }
void HardFault_Handler(void)  { for (;;) { } }
void MemManage_Handler(void)  { for (;;) { } }
void BusFault_Handler(void)   { for (;;) { } }
void UsageFault_Handler(void) { for (;;) { } }

void SVC_Handler(void)        { }
void DebugMon_Handler(void)   { }
void PendSV_Handler(void)     { }
