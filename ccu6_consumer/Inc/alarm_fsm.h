/**
 * @file    alarm_fsm.h
 * @brief   May trang thai canh bao va chinh sach dau ra.
 */
#ifndef ALARM_FSM_H
#define ALARM_FSM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    C_SYS_OFF = 0,
    C_WARMUP,
    C_L0,
    C_L1,
    C_L2,
    C_L3,
    C_SENSOR_FAULT,
    C_LINK_LOST
} consumer_state_t;

typedef struct {
    bool blue;
    bool yellow;
    bool red;
    bool green;
    bool buzzer;
    bool relay_assert;
} alarm_outputs_t;

consumer_state_t alarm_resolve_state(bool sys_on, char code,
                                     uint32_t ms_since_frame,
                                     uint32_t ms_since_stop_mismatch);

uint32_t alarm_blink_half_ms(consumer_state_t st, uint32_t l3_hz);

uint32_t alarm_next_l3_hz(uint32_t hz);

void alarm_outputs(consumer_state_t st, bool blink_on, bool beep_on,
                   alarm_outputs_t *out);

#endif
