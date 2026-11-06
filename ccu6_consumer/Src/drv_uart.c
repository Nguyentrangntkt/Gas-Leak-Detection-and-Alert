/**
 * @file    drv_uart.c
 * @brief   USART2 lien ket voi F103, ring buffer + ngat.
 */
#include "stm32f4xx.h"
#include "drv_uart.h"
#include "bsp_clock.h"
#include "config.h"
#include "ring.h"

_Static_assert(CFG_UART_TX_RING > 1u && CFG_UART_TX_RING <= UINT16_MAX, "TX ring size");
_Static_assert(CFG_UART_RX_RING > 1u && CFG_UART_RX_RING <= UINT16_MAX, "RX ring size");

static char     s_tx[CFG_UART_TX_RING];
static volatile uint16_t s_tx_head, s_tx_tail;
static char     s_rx[CFG_UART_RX_RING];
static volatile uint16_t s_rx_head, s_rx_tail;
static volatile uint32_t s_errors, s_dropped;
static volatile bool s_rx_gap;

void drv_uart_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    s_tx_head = 0u; s_tx_tail = 0u;
    s_rx_head = 0u; s_rx_tail = 0u;
    s_errors  = 0u; s_dropped = 0u;
    s_rx_gap = false;

    USART2->BRR = (uint32_t)((bsp_pclk1_hz() + (CFG_UART_BAUD / 2u)) /
                             CFG_UART_BAUD);
    USART2->CR2 = 0u;
    USART2->CR3 = 0u;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

    NVIC_SetPriority(USART2_IRQn, 1u);
    NVIC_EnableIRQ(USART2_IRQn);
}

bool drv_uart_write(const char *data, int len)
{
    if (!ring_write_frame(s_tx, &s_tx_head, &s_tx_tail,
                          CFG_UART_TX_RING, data, len)) {
        s_dropped++;
        return false;
    }
    USART2->CR1 |= USART_CR1_TXEIE;
    return true;
}

uart_rx_event_t drv_uart_read_event(char *out)
{
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uart_rx_event_t event = UART_RX_NONE;
    if (s_rx_gap) {
        s_rx_tail = s_rx_head;
        s_rx_gap = false;
        event = UART_RX_GAP;
    } else if (ring_read_byte(s_rx, &s_rx_head, &s_rx_tail,
                              CFG_UART_RX_RING, out)) {
        event = UART_RX_BYTE;
    }
    __set_PRIMASK(mask);
    return event;
}

uint32_t drv_uart_error_count(void) { return s_errors;  }
uint32_t drv_uart_dropped(void)     { return s_dropped; }

void drv_uart_irq(void)
{
    const uint32_t sr = USART2->SR;

    if (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        (void)USART2->DR;
        s_errors++;
        s_rx_gap = true;
        return;
    }

    if (sr & USART_SR_RXNE) {
        const char c = (char)(USART2->DR & 0xFFu);
        const uint16_t next = ring_next(s_rx_head, CFG_UART_RX_RING);
        if (next != s_rx_tail) {
            s_rx[s_rx_head] = c;
            atomic_signal_fence(memory_order_release);
            s_rx_head = next;
        } else {
            s_errors++;
            s_rx_gap = true;
        }
    }

    if ((sr & USART_SR_TXE) && (USART2->CR1 & USART_CR1_TXEIE)) {
        char c;
        if (!ring_read_byte(s_tx, &s_tx_head, &s_tx_tail,
                            CFG_UART_TX_RING, &c)) {
            USART2->CR1 &= ~USART_CR1_TXEIE;
        } else {
            USART2->DR = (uint8_t)c;
        }
    }
}
