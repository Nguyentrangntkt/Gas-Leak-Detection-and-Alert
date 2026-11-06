/**
 * @file    drv_uart.c
 * @brief   USART1 (lien ket F401) va USART2 (toi ESP32), ring buffer + ngat.
 */
#include "stm32f1xx.h"
#include "drv_uart.h"
#include "bsp_clock.h"
#include "config.h"
#include "ring.h"

_Static_assert(CFG_UART_TX_RING > 1u && CFG_UART_TX_RING <= UINT16_MAX, "TX ring size");
_Static_assert(CFG_UART_RX_RING > 1u && CFG_UART_RX_RING <= UINT16_MAX, "RX ring size");

typedef struct {
    USART_TypeDef *regs;
    char           tx[CFG_UART_TX_RING];
    volatile uint16_t tx_head;
    volatile uint16_t tx_tail;
    char           rx[CFG_UART_RX_RING];
    volatile uint16_t rx_head;
    volatile uint16_t rx_tail;
    volatile uint32_t errors;
    volatile uint32_t dropped;
    volatile bool  rx_gap;
    bool           has_rx;
} uart_ctx_t;

static uart_ctx_t s_uart[UART_COUNT];

static void uart_hw_init(uart_ctx_t *u, uint32_t pclk, bool enable_rx)
{

    u->regs->BRR = (uint32_t)((pclk + (CFG_UART_BAUD / 2u)) / CFG_UART_BAUD);

    uint32_t cr1 = USART_CR1_UE | USART_CR1_TE;
    if (enable_rx) {
        cr1 |= USART_CR1_RE | USART_CR1_RXNEIE;
    }
    u->regs->CR1 = cr1;
    u->regs->CR2 = 0u;
    u->regs->CR3 = 0u;
    u->has_rx = enable_rx;
}

void drv_uart_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    for (int i = 0; i < (int)UART_COUNT; i++) {
        s_uart[i].tx_head = 0u;
        s_uart[i].tx_tail = 0u;
        s_uart[i].rx_head = 0u;
        s_uart[i].rx_tail = 0u;
        s_uart[i].rx_gap = false;
        s_uart[i].errors  = 0u;
        s_uart[i].dropped = 0u;
    }

    s_uart[UART_LINK].regs  = USART1;
    s_uart[UART_ESP32].regs = USART2;

    uart_hw_init(&s_uart[UART_LINK],  bsp_pclk2_hz(), true);
    uart_hw_init(&s_uart[UART_ESP32], bsp_pclk1_hz(), false);

    NVIC_SetPriority(USART1_IRQn, 1u);
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_SetPriority(USART2_IRQn, 1u);
    NVIC_EnableIRQ(USART2_IRQn);
}

bool drv_uart_write(uart_id_t id, const char *data, int len)
{
    uart_ctx_t *u = &s_uart[id];

    if (!ring_write_frame(u->tx, &u->tx_head, &u->tx_tail,
                          CFG_UART_TX_RING, data, len)) {
        u->dropped++;
        return false;
    }

    u->regs->CR1 |= USART_CR1_TXEIE;
    return true;
}

uart_rx_event_t drv_uart_read_event(uart_id_t id, char *out)
{
    uart_ctx_t *u = &s_uart[id];
    const uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uart_rx_event_t event = UART_RX_NONE;
    if (u->rx_gap) {

        u->rx_tail = u->rx_head;
        u->rx_gap = false;
        event = UART_RX_GAP;
    } else if (ring_read_byte(u->rx, &u->rx_head, &u->rx_tail,
                              CFG_UART_RX_RING, out)) {
        event = UART_RX_BYTE;
    }
    __set_PRIMASK(mask);
    return event;
}

uint32_t drv_uart_error_count(uart_id_t id) { return s_uart[id].errors;  }
uint32_t drv_uart_dropped(uart_id_t id)     { return s_uart[id].dropped; }

void drv_uart_irq(uart_id_t id)
{
    uart_ctx_t *u  = &s_uart[id];
    const uint32_t sr = u->regs->SR;

    if (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
        (void)u->regs->DR;
        u->errors++;
        u->rx_gap = true;
        return;
    }

    if ((sr & USART_SR_RXNE) && u->has_rx) {
        const char c = (char)(u->regs->DR & 0xFFu);
        const uint16_t next = ring_next(u->rx_head, CFG_UART_RX_RING);
        if (next != u->rx_tail) {
            u->rx[u->rx_head] = c;
            atomic_signal_fence(memory_order_release);
            u->rx_head = next;
        } else {
            u->errors++;
            u->rx_gap = true;
        }
    }

    if ((sr & USART_SR_TXE) && (u->regs->CR1 & USART_CR1_TXEIE)) {
        char c;
        if (!ring_read_byte(u->tx, &u->tx_head, &u->tx_tail,
                            CFG_UART_TX_RING, &c)) {
            u->regs->CR1 &= ~USART_CR1_TXEIE;
        } else {
            u->regs->DR = (uint8_t)c;
        }
    }
}
