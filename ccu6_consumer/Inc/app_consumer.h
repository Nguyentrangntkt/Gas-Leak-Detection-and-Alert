/**
 * @file    app_consumer.h
 * @brief   API vong dieu khien cua Consumer.
 */
#ifndef APP_CONSUMER_H
#define APP_CONSUMER_H

#include "alarm_fsm.h"

void             app_consumer_init(void);
void             app_consumer_task(void);
consumer_state_t app_consumer_state(void);

#endif
