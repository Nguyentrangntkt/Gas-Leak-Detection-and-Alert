/**
 * @file    drv_uart.h
 * @brief   USART phi chan voi ring buffer cho Producer.
 */
#ifndef DRV_UART_H
#define DRV_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "uart_rx_event.h"

typedef enum {
    UART_LINK = 0,
    UART_ESP32,
    UART_COUNT
} uart_id_t;

void     drv_uart_init(void);

bool     drv_uart_write(uart_id_t id, const char *data, int len);

uart_rx_event_t drv_uart_read_event(uart_id_t id, char *out);

uint32_t drv_uart_error_count(uart_id_t id);

uint32_t drv_uart_dropped(uart_id_t id);

void     drv_uart_irq(uart_id_t id);

#endif
