/**
 * @file    app_consumer.c
 * @brief   Vong dieu khien Consumer: nhan trang thai, chay FSM, dieu khien dau ra.
 */
#include "app_consumer.h"
#include "config.h"
#include "protocol.h"
#include "alarm_fsm.h"
#include "drv_uart.h"
#include "drv_tim.h"
#include "hal_output.h"
#include "hal_switch.h"
#include "bsp_systick.h"

static consumer_state_t s_state;
static bool        s_sys_on;
static char        s_code;
static uint32_t    s_last_frame_ms;
static uint32_t    s_last_cmd_ms;

/* Cong tac dang ON nhung Producer van bao 'O'. Qua CFG_SYNC_TIMEOUT_MS
 * thi coi la mat dong bo chu khong phai dang khoi dong. */
static bool        s_mismatch;
static uint32_t    s_mismatch_since_ms;
static proto_rx_t  s_rx;

static uint32_t s_blink_tick;
static bool     s_blink_on;
static uint32_t s_blink_hz;
static uint32_t s_beep_tick;

static void send_cmd(bool start)
{
    char frame[4];
    const int n = proto_build_cmd(frame, (int)sizeof(frame),
                                  start ? PROTO_CMD_START : PROTO_CMD_STOP);
    if (n > 0) {

        (void)drv_uart_write(frame, n);
    }
    s_last_cmd_ms = millis();
}

static void handle_rx(void)
{
    char c;
    uart_rx_event_t event;
    while ((event = drv_uart_read_event(&c)) != UART_RX_NONE) {
        if (event == UART_RX_GAP) {
            proto_rx_discard(&s_rx);
            continue;
        }
        if (!proto_rx_push(&s_rx, c)) {
            continue;
        }

        char code;
        if (proto_parse_status(s_rx.buf, s_rx.len, &code)) {
            s_code = code;
            s_last_frame_ms = millis();
        }

        proto_rx_init(&s_rx);
    }
}

static void blink_update(consumer_state_t st)
{
    const uint32_t half = alarm_blink_half_ms(st, s_blink_hz);
    const uint32_t now  = drv_tim_ticks();

    if (half == 0u) {
        s_blink_on   = true;
        s_blink_tick = now;
        return;
    }

    if ((uint32_t)(now - s_blink_tick) < half) {
        return;
    }
    s_blink_tick = now;
    s_blink_on   = !s_blink_on;

    if (st == C_L3 && !s_blink_on) {
        s_blink_hz = alarm_next_l3_hz(s_blink_hz);
    }
}

static bool beep_phase(void)
{
    const uint32_t now = drv_tim_ticks();
    if ((uint32_t)(now - s_beep_tick) >= CFG_FAULT_BEEP_PERIOD_MS) {
        s_beep_tick = now;
    }
    return ((uint32_t)(now - s_beep_tick) < CFG_FAULT_BEEP_ON_MS);
}

static void drive_outputs(consumer_state_t st)
{
    alarm_outputs_t out;
    alarm_outputs(st, s_blink_on, beep_phase(), &out);

    hal_led_env(out.blue, out.yellow, out.red);
    hal_led_green(out.green);
    hal_buzzer(out.buzzer);
    if (out.relay_assert) {
        hal_relay_assert();
    } else {
        hal_relay_release();
    }
}

void app_consumer_init(void)
{
    proto_rx_init(&s_rx);

    s_code       = PROTO_CODE_NONE;
    s_blink_tick = drv_tim_ticks();
    s_beep_tick  = drv_tim_ticks();
    s_blink_on   = false;
    s_blink_hz   = CFG_BLINK_L3_HZ_MIN;

    s_mismatch          = false;
    s_mismatch_since_ms = 0u;

    s_sys_on        = hal_switch_is_on();
    s_last_frame_ms = millis();
    s_state         = s_sys_on ? C_LINK_LOST : C_SYS_OFF;

    drive_outputs(s_state);
    send_cmd(s_sys_on);
}

void app_consumer_task(void)
{
    handle_rx();

    bool on;
    if (hal_switch_poll(&on)) {
        s_sys_on = on;
        send_cmd(on);

        s_code = PROTO_CODE_NONE;
    }

    if ((uint32_t)(millis() - s_last_cmd_ms) >= CFG_CMD_HEARTBEAT_MS) {
        send_cmd(s_sys_on);
    }

    if (s_sys_on && s_code == PROTO_CODE_STOPPED) {
        if (!s_mismatch) {
            s_mismatch          = true;
            s_mismatch_since_ms = millis();
        }
    } else {
        s_mismatch = false;
    }

    const uint32_t mismatch_age =
        s_mismatch ? (uint32_t)(millis() - s_mismatch_since_ms) : 0u;

    const consumer_state_t next =
        alarm_resolve_state(s_sys_on, s_code,
                            (uint32_t)(millis() - s_last_frame_ms),
                            mismatch_age);

    if (next != s_state) {
        s_state = next;
        if (next == C_L3) {
            s_blink_hz = CFG_BLINK_L3_HZ_MIN;
        }
        s_blink_tick = drv_tim_ticks();
        s_blink_on   = true;
    }

    blink_update(s_state);
    drive_outputs(s_state);
}

consumer_state_t app_consumer_state(void)
{
    return s_state;
}
