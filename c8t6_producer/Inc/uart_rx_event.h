/**
 * @file    uart_rx_event.h
 * @brief   Ket qua doc UART. BAN SAO DONG BO voi ccu6_consumer/Inc/.
 */
#ifndef UART_RX_EVENT_H
#define UART_RX_EVENT_H

typedef enum {
    UART_RX_NONE = 0,
    UART_RX_BYTE,
    UART_RX_GAP
} uart_rx_event_t;

#endif
