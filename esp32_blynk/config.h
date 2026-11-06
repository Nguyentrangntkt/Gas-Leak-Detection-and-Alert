/**
 * @file    config.h
 * @brief   Cau hinh khong bi mat cua module ESP32.
 */
#ifndef ESP32_CONFIG_H
#define ESP32_CONFIG_H

#define CFG_UART_RX_PIN        16
#define CFG_UART_TX_PIN        17
#define CFG_UART_BAUD          9600

#define CFG_VPIN_PPM           V0
#define CFG_VPIN_LEVEL         V1
#define CFG_VPIN_ONLINE        V2

#define CFG_VPIN_STATUS        V3

#define CFG_BLYNK_EVENT        "gas_alert"

#define CFG_BLYNK_EVENT_FAULT  "sensor_fault"

#define CFG_FAULT_COOLDOWN_MS  300000UL

#define CFG_ALERT_FROM_LEVEL   2

#define CFG_ALERT_COOLDOWN_MS  60000UL

#define CFG_NET_RETRY_MIN_MS        2000UL
#define CFG_NET_RETRY_MAX_MS        60000UL

#define CFG_BLYNK_CONNECT_MS        500UL

#define CFG_UART_RX_BUFFER          512

#define CFG_LINK_TIMEOUT_MS    5000UL

#define CFG_PUSH_PERIOD_MS     1000UL

#endif
