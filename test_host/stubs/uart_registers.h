/**
 * @file    uart_registers.h
 * @brief   Thanh ghi gia lap cho host: chi la bo nho, KHONG mo phong
 * side effect cua SR/DR, thoi gian NVIC hay hanh vi dien.
 */
#ifndef UART_REGISTERS_H
#define UART_REGISTERS_H

#include <stdint.h>
typedef struct { volatile uint32_t SR, DR, BRR, CR1, CR2, CR3; } USART_TypeDef;
typedef struct { volatile uint32_t APB1ENR, APB2ENR, CSR; } RCC_TypeDef;
typedef struct { volatile uint32_t KR, PR, RLR; } IWDG_TypeDef;
typedef struct { volatile uint32_t APB1FZ; } DBGMCU_TypeDef;
extern IWDG_TypeDef host_iwdg;
extern DBGMCU_TypeDef host_dbgmcu;
extern USART_TypeDef host_usart1, host_usart2;
extern RCC_TypeDef host_rcc;
extern uint32_t host_primask;
#define USART1 (&host_usart1)
#define USART2 (&host_usart2)
#define RCC (&host_rcc)
#define IWDG (&host_iwdg)
#define DBGMCU (&host_dbgmcu)
#define RCC_CSR_WDGRSTF (1u << 29)
#define RCC_CSR_RMVF (1u << 24)
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP (1u << 12)
#define RCC_APB1ENR_USART2EN (1u << 17)

/* USART1EN nam o bit KHAC NHAU giua hai ho: F103 bit 14, F401 bit 4. */
#ifdef TEST_UART_PRODUCER
#define RCC_APB2ENR_USART1EN (1u << 14)
#else
#define RCC_APB2ENR_USART1EN (1u << 4)
#endif
#define USART_CR1_UE (1u << 13)
#define USART_CR1_TE (1u << 3)
#define USART_CR1_RE (1u << 2)
#define USART_CR1_RXNEIE (1u << 5)
#define USART_CR1_TXEIE (1u << 7)
#define USART_SR_PE (1u << 0)
#define USART_SR_FE (1u << 1)
#define USART_SR_NE (1u << 2)
#define USART_SR_ORE (1u << 3)
#define USART_SR_RXNE (1u << 5)
#define USART_SR_TXE (1u << 7)
#define USART1_IRQn 37
#define USART2_IRQn 38
static inline void NVIC_SetPriority(int irq, uint32_t p) { (void)irq; (void)p; }
static inline void NVIC_EnableIRQ(int irq) { (void)irq; }
static inline uint32_t __get_PRIMASK(void) { return host_primask; }
static inline void __disable_irq(void) { host_primask = 1u; }
static inline void __set_PRIMASK(uint32_t p) { host_primask = p; }
#endif
