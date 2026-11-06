/**
 * @file    alarm_fsm.c
 * @brief   May trang thai canh bao. Thuan tinh toan, kiem thu duoc tren PC.
 */
#include "alarm_fsm.h"
#include "config.h"
#include "protocol.h"

_Static_assert(CFG_BUZZER_FROM_LEVEL == 2u || CFG_BUZZER_FROM_LEVEL == 3u,
               "buzzer threshold supports level 2 or 3");

consumer_state_t alarm_resolve_state(bool sys_on, char code,
                                     uint32_t ms_since_frame,
                                     uint32_t ms_since_stop_mismatch)
{

    if (!sys_on) {
        return C_SYS_OFF;
    }

    if (ms_since_frame >= CFG_LINK_TIMEOUT_MS) {
        return C_LINK_LOST;
    }

    switch (code) {
    case PROTO_CODE_L0:      return C_L0;
    case PROTO_CODE_L1:      return C_L1;
    case PROTO_CODE_L2:      return C_L2;
    case PROTO_CODE_L3:      return C_L3;
    case PROTO_CODE_FAULT:   return C_SENSOR_FAULT;
    case PROTO_CODE_WARMUP:  return C_WARMUP;

    case PROTO_CODE_STOPPED:
        return (ms_since_stop_mismatch >= CFG_SYNC_TIMEOUT_MS)
             ? C_LINK_LOST : C_WARMUP;
    default:                 return C_LINK_LOST;
    }
}

uint32_t alarm_blink_half_ms(consumer_state_t st, uint32_t l3_hz)
{
    switch (st) {
    case C_L2:
        return CFG_BLINK_L2_HALF_MS;
    case C_L3:
        if (l3_hz < CFG_BLINK_L3_HZ_MIN) {
            l3_hz = CFG_BLINK_L3_HZ_MIN;
        }
        return CFG_BLINK_L3_BASE_MS / l3_hz;
    case C_SENSOR_FAULT:
        return CFG_BLINK_FAULT_HALF_MS;
    case C_LINK_LOST:
    case C_WARMUP:
        return CFG_BLINK_LINK_HALF_MS;
    default:
        return 0u;
    }
}

uint32_t alarm_next_l3_hz(uint32_t hz)
{
    hz++;
    if (hz > CFG_BLINK_L3_HZ_MAX) {
        hz = CFG_BLINK_L3_HZ_MIN;
    }
    return hz;
}

static uint8_t state_to_level(consumer_state_t st)
{
    switch (st) {
    case C_L1: return 1u;
    case C_L2: return 2u;
    case C_L3: return 3u;
    default:   return 0u;
    }
}

void alarm_outputs(consumer_state_t st, bool blink_on, bool beep_on,
                   alarm_outputs_t *out)
{
    out->blue         = false;
    out->yellow       = false;
    out->red          = false;
    out->green        = false;
    out->buzzer       = false;
    out->relay_assert = false;

    switch (st) {
    case C_SYS_OFF:
        out->green = true;
        break;

    case C_WARMUP:
        out->blue = blink_on;
        break;

    case C_L0:
        out->blue = true;
        break;

    case C_L1:
        out->yellow = true;
        break;

    case C_L2:
    case C_L3:
        out->red          = blink_on;
        out->buzzer       = (state_to_level(st) >= CFG_BUZZER_FROM_LEVEL);
        out->relay_assert = (state_to_level(st) >= CFG_RELAY_ASSERT_FROM_LEVEL);
        break;

    case C_SENSOR_FAULT:

        out->yellow       = blink_on;
        out->red          = !blink_on;
        out->buzzer       = beep_on;
        out->relay_assert = true;
        break;

    case C_LINK_LOST:
    default:
        out->blue         = blink_on;
        out->yellow       = blink_on;
        out->red          = blink_on;
        out->buzzer       = beep_on;
        out->relay_assert = true;
        break;
    }
}
