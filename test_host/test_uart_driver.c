/**
 * @file    test_uart_driver.c
 * @brief   Kiem driver UART that tren thanh ghi gia lap. Dich hai lan, moi MCU mot lan.
 */
#include "test_util.h"
#include "uart_registers.h"
#include "drv_uart.h"
#include "protocol.h"
#include "config.h"
#include <string.h>
USART_TypeDef host_usart1, host_usart2;
RCC_TypeDef host_rcc;
uint32_t host_primask;
uint32_t bsp_pclk1_hz(void) { return 8000000u; }
uint32_t bsp_pclk2_hz(void) { return 8000000u; }
#ifdef TEST_UART_PRODUCER
#define HW USART1
#define WRITE(p,n) drv_uart_write(UART_LINK,p,n)
#define READ(p) drv_uart_read_event(UART_LINK,p)
#define IRQ() drv_uart_irq(UART_LINK)
#define ERRORS() drv_uart_error_count(UART_LINK)
#define DROPPED() drv_uart_dropped(UART_LINK)
#else
#define HW USART2
#define WRITE(p,n) drv_uart_write(p,n)
#define READ(p) drv_uart_read_event(p)
#define IRQ() drv_uart_irq()
#define ERRORS() drv_uart_error_count()
#define DROPPED() drv_uart_dropped()
#endif
static void receive(char c, uint32_t flags)
{
    HW->DR = (uint8_t)c;
    HW->SR = USART_SR_RXNE | flags;
    IRQ();
    HW->SR = 0u;
}
static void test_gap(void)
{
    SUITE("Real UART driver: errors/overflow -> GAP, queue flush, IRQ mask");
    const uint32_t flags[] = { USART_SR_ORE, USART_SR_NE, USART_SR_FE, USART_SR_PE };
    for (unsigned i = 0; i < sizeof(flags)/sizeof(flags[0]); i++) {
        drv_uart_init();
        proto_rx_t rx; proto_rx_init(&rx);
        char c = '?';
        receive('S', 0u); CHECK_EQ_I(READ(&c), UART_RX_BYTE, "prefix byte");
        CHECK(!proto_rx_push(&rx, c), "partial prefix");
        receive('x', flags[i]); receive('0', 0u); receive('\n', 0u);
        host_primask = i & 1u;
        CHECK_EQ_I(READ(&c), UART_RX_GAP, "error event before queued suffix");
        CHECK_EQ_I(host_primask, i & 1u, "restore original PRIMASK");
        proto_rx_discard(&rx);
        CHECK_EQ_I(READ(&c), UART_RX_NONE, "flush even queued delimiters");
        CHECK_EQ_I(ERRORS(), 1, "hardware error counted");
        const char *stream = "S0\nS3\n";
        unsigned frames = 0; char code = '?';
        for (const char *p = stream; *p; p++) {
            receive(*p, 0u);
            if (READ(&c) == UART_RX_BYTE && proto_rx_push(&rx, c)) {
                if (proto_parse_status(rx.buf, rx.len, &code)) { frames++; }
                proto_rx_init(&rx);
            }
        }
        CHECK_EQ_I(frames, 1, "first fresh newline resync; next frame accepted");
        CHECK_EQ_I(code, '3', "damaged stream cannot fabricate S0");
    }
    drv_uart_init();
    for (unsigned i = 0; i < CFG_UART_RX_RING; i++) { receive('x', 0u); }
    char c;
    CHECK_EQ_I(ERRORS(), 1, "RX capacity size-1 then overflow");
    CHECK_EQ_I(READ(&c), UART_RX_GAP, "ring overflow is GAP");
    CHECK_EQ_I(READ(&c), UART_RX_NONE, "overflow queue flushed");
    host_primask = 0u;
}
static void test_tx(void)
{
    SUITE("Real UART driver: whole-frame enqueue, IRQ transmission, wrap");
    drv_uart_init();
    CHECK(WRITE("", 0), "empty write");
    unsigned accepted = 0u;
    while (WRITE("S2\n", 3)) { accepted++; }
    CHECK_EQ_I(accepted, (CFG_UART_TX_RING-1u)/3u, "frame capacity");
    CHECK_EQ_I(DROPPED(), 1, "one rejected whole frame");
    char output[CFG_UART_TX_RING]; unsigned n = 0u;
    while (HW->CR1 & USART_CR1_TXEIE) {
        HW->SR = USART_SR_TXE; IRQ();
        if (HW->CR1 & USART_CR1_TXEIE) { output[n++] = (char)HW->DR; }
    }
    CHECK_EQ_I(n, accepted * 3u, "no partial rejected frame");
    bool correct = true;
    for (unsigned i = 0u; i < n; i++) { if (output[i] != "S2\n"[i%3u]) { correct=false; } }
    CHECK(correct, "exact ordered bytes");
    CHECK(WRITE("S3\n", 3), "enqueue across physical wrap");
    for (unsigned i=0u; i<3u; i++) {
        HW->SR = USART_SR_TXE; IRQ();
        CHECK_EQ_I(HW->DR, (uint8_t)"S3\n"[i], "wrap byte order");
    }
#ifdef TEST_UART_PRODUCER
    CHECK(drv_uart_write(UART_ESP32, "P00000,W\n", 9), "independent telemetry TX");
#endif
}
int main(void)
{
    test_gap();
    test_tx();

#ifdef TEST_UART_PRODUCER
    return test_report("test_uart_driver_f103");
#else
    return test_report("test_uart_driver_f401");
#endif
}
