/**
 * @file    drv_uart.h
 * @brief   USART2 (PA2/PA3) phi chan, ring buffer.
 */
#ifndef DRV_UART_H
#define DRV_UART_H

#include <stdint.h>
#include <stdbool.h>
#include "uart_rx_event.h"

void     drv_uart_init(void);

bool     drv_uart_write(const char *data, int len);

uart_rx_event_t drv_uart_read_event(char *out);

uint32_t drv_uart_error_count(void);

uint32_t drv_uart_dropped(void);

void     drv_uart_irq(void);

#endif
