/**
 * @file    app_producer.h
 * @brief   API vong dieu khien cua Producer.
 */
#ifndef APP_PRODUCER_H
#define APP_PRODUCER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    P_WARMUP = 0,
    P_RUN,
    P_STOPPED,
    P_SENSOR_FAULT
} producer_state_t;

void             app_producer_init(void);
void             app_producer_task(void);
producer_state_t app_producer_state(void);

#endif
