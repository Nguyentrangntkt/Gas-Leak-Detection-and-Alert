/**
 * @file    app_producer.c
 * @brief   Vong dieu khien cua Producer: do - loc - phan muc - truyen - hien thi.
 */
#include "app_producer.h"
#include "config.h"
#include "protocol.h"
#include "mq2_sensor.h"
#include "drv_adc.h"
#include "drv_uart.h"
#include "drv_lcd1602.h"
#include "bsp_systick.h"
#include "bsp_clock.h"

static producer_state_t s_state;
static bool        s_system_on;
static char        s_code;
static char        s_code_sent;
static gas_level_t s_level;
static float       s_ppm;
static uint8_t     s_ok_cycles;

/* Suc khoe cam bien, DOC LAP voi cong tac bat/tat: Stop/Start khong
 * xoa duoc lich su nay. */
static bool        s_fault_latched;

/* Tuoi cua phep do: chi cong bo muc gas khi da tung do hop le VA so do
 * gan nhat con tre hon CFG_MEAS_MAX_AGE_MS. */
static bool        s_meas_valid;
static uint32_t    s_last_ok_ms;
static uint32_t    s_last_sample_ms;
static uint32_t    s_last_tx_ms;
static uint32_t    s_last_cmd_ms;
static uint32_t    s_last_lcd_try_ms;
static bool        s_link_down;
static proto_rx_t  s_rx;

static uint16_t s_raw[CFG_SAMPLE_COUNT];
static float    s_scratch[CFG_SAMPLE_COUNT];

#ifdef CFG_SIMULATE_PPM
static float s_sim_ppm = CFG_SIM_PPM_MIN;
static float s_sim_dir = 1.0f;
#endif

static void fmt_u32(char *dst, uint32_t v, int width)
{
    for (int i = width - 1; i >= 0; i--) {
        dst[i] = (char)('0' + (v % 10u));
        v /= 10u;
    }
}

static void send_status(void)
{
    char frame[4];
    const int n = proto_build_status(frame, (int)sizeof(frame), s_code);

    if ((n > 0) && drv_uart_write(UART_LINK, frame, n)) {
        s_code_sent = s_code;
    }
}

static void send_telemetry(void)
{
    char frame[12];
    uint32_t ppm_i = 0u;
    if (s_ppm > 0.0f) {
        ppm_i = (s_ppm >= 99999.0f) ? 99999u : (uint32_t)s_ppm;
    }
    const int n = proto_build_telemetry(frame, (int)sizeof(frame), ppm_i, s_code);
    if (n > 0) {

        (void)drv_uart_write(UART_ESP32, frame, n);
    }
}

static void lcd_update(void)
{
    char l0[CFG_LCD_COLS + 1u];
    char l1[CFG_LCD_COLS + 1u];

    for (uint8_t i = 0u; i < CFG_LCD_COLS; i++) {
        l0[i] = ' ';
        l1[i] = ' ';
    }
    l0[CFG_LCD_COLS] = '\0';
    l1[CFG_LCD_COLS] = '\0';

    switch (s_state) {
    case P_WARMUP: {
        const uint32_t now = millis();
        const uint32_t left = (now < CFG_WARMUP_MS)
                            ? ((CFG_WARMUP_MS - now + 999u) / 1000u) : 0u;
        const char warm[] = "WARMING UP";
        for (int i = 0; warm[i] != '\0'; i++) {
            l0[i] = warm[i];
        }
        fmt_u32(&l0[11], left, 3u);
        l0[14] = 's';
        break;
    }
    case P_SENSOR_FAULT: {
        const char err[] = "SENSOR ERROR";
        for (int i = 0; err[i] != '\0'; i++) {
            l0[i] = err[i];
        }
        break;
    }
    case P_STOPPED: {

        const char off[] = "SYSTEM OFF";
        for (int i = 0; off[i] != '\0'; i++) {
            l0[i] = off[i];
        }
        break;
    }
    default: {
        const char tag[] = "Gas:";
        for (int i = 0; tag[i] != '\0'; i++) {
            l0[i] = tag[i];
        }
        uint32_t ppm_i = (s_ppm > 0.0f)
                       ? ((s_ppm >= 99999.0f) ? 99999u : (uint32_t)s_ppm) : 0u;
        fmt_u32(&l0[4], ppm_i, 5);
        l0[10] = 'p';
        l0[11] = 'p';
        l0[12] = 'm';
        break;
    }
    }

    const char s2[] = "SysS:";
    for (int i = 0; s2[i] != '\0'; i++) {
        l1[i] = s2[i];
    }
    l1[5] = s_system_on ? '1' : '0';

    const char e2[] = " EnvS:";
    for (int i = 0; e2[i] != '\0'; i++) {
        l1[6 + i] = e2[i];
    }
    l1[12] = (s_state == P_SENSOR_FAULT) ? 'F'
           : (s_state == P_WARMUP)       ? 'W'
           : (char)('0' + s_level.level);

    if (!bsp_clock_confirmed()) {
        l1[14] = 'C';
        l1[15] = '?';
    } else if (s_link_down) {
        l1[14] = 'L';
        l1[15] = '?';
    } else if (bsp_clock_degraded()) {
        l1[14] = 'H';
        l1[15] = 'S';
    }

    drv_lcd_print_line(0u, l0);
    drv_lcd_print_line(1u, l1);
}

static void advance_state(void)
{
    if (!s_system_on) {
        s_state = P_STOPPED;
        return;
    }

    if (s_state == P_STOPPED) {

        if (s_fault_latched) {
            s_state = P_SENSOR_FAULT;
        } else {
            s_state = (millis() < CFG_WARMUP_MS) ? P_WARMUP : P_RUN;
        }
    }

    if (s_state == P_WARMUP && millis() >= CFG_WARMUP_MS) {
        s_state = P_RUN;
    }
}

static void refresh_code(void)
{
    switch (s_state) {
    case P_STOPPED:      s_code = PROTO_CODE_STOPPED; break;
    case P_WARMUP:       s_code = PROTO_CODE_WARMUP;  break;
    case P_SENSOR_FAULT: s_code = PROTO_CODE_FAULT;   break;
    default:

        if (meas_is_fresh(s_meas_valid, millis(), s_last_ok_ms,
                          CFG_MEAS_MAX_AGE_MS)) {
            s_code = (char)(PROTO_CODE_L0 + s_level.level);
        } else if (s_fault_latched) {
            s_code = PROTO_CODE_FAULT;
        } else {

            s_code = PROTO_CODE_WARMUP;
        }
        break;
    }
}

static void handle_rx(void)
{
    char c;
    uart_rx_event_t event;
    while ((event = drv_uart_read_event(UART_LINK, &c)) != UART_RX_NONE) {
        if (event == UART_RX_GAP) {
            proto_rx_discard(&s_rx);
            continue;
        }
        if (!proto_rx_push(&s_rx, c)) {
            continue;
        }

        char cmd;
        if (proto_parse_cmd(s_rx.buf, s_rx.len, &cmd)) {
            s_last_cmd_ms = millis();
            s_link_down   = false;

            if (cmd == PROTO_CMD_START && !s_system_on) {
                s_system_on = true;

                advance_state();
                refresh_code();
                send_status();
            } else if (cmd == PROTO_CMD_STOP && s_system_on) {
                s_system_on = false;
                advance_state();
                refresh_code();
                send_status();
            }
        }
        proto_rx_init(&s_rx);
    }
}

static void sample_cycle(void)
{
#ifdef CFG_SIMULATE_PPM
    s_sim_ppm += s_sim_dir * CFG_SIM_PPM_STEP;
    if (s_sim_ppm >= CFG_SIM_PPM_MAX) {
        s_sim_ppm = CFG_SIM_PPM_MAX;
        s_sim_dir = -1.0f;
    } else if (s_sim_ppm <= CFG_SIM_PPM_MIN) {
        s_sim_ppm = CFG_SIM_PPM_MIN;
        s_sim_dir = 1.0f;
    }
    s_ppm           = s_sim_ppm;
    s_ok_cycles     = CFG_SENSOR_FAULT_CYCLES;
    s_fault_latched = false;
    s_meas_valid    = true;
    s_last_ok_ms    = millis();
    (void)gas_level_update(&s_level, s_ppm);
    if (s_state == P_SENSOR_FAULT) {
        s_state = P_RUN;
    }
    return;
#else

    bool adc_ok = true;
    for (int i = 0; i < CFG_SAMPLE_COUNT; i++) {
        if (!drv_adc_read(&s_raw[i])) {
            adc_ok = false;
            break;
        }
        if (i + 1 < CFG_SAMPLE_COUNT) {
            delay_ms(CFG_SAMPLE_GAP_MS);
        }
    }

    mq2_result_t res;
    const bool meas_ok =
        adc_ok && (mq2_process(s_raw, CFG_SAMPLE_COUNT, s_scratch, &res) == MQ2_OK);

    if (!meas_ok) {
        s_ok_cycles = 0u;

        gas_level_invalidate(&s_level);

        if (!meas_is_fresh(s_meas_valid, millis(), s_last_ok_ms,
                           CFG_MEAS_MAX_AGE_MS)) {
            s_fault_latched = true;
            s_state         = P_SENSOR_FAULT;
        }
        return;
    }

    if (s_ok_cycles < CFG_SENSOR_FAULT_CYCLES) {
        s_ok_cycles++;
    }
    s_ppm        = res.ppm;
    s_meas_valid = true;
    s_last_ok_ms = millis();
    (void)gas_level_update(&s_level, s_ppm);

    if (s_fault_latched && s_ok_cycles >= CFG_SENSOR_FAULT_CYCLES) {
        s_fault_latched = false;
    }
    if (s_state == P_SENSOR_FAULT && !s_fault_latched) {
        s_state = P_RUN;
    }
#endif
}

void app_producer_init(void)
{
    s_state        = P_WARMUP;
    s_system_on    = true;
    s_code         = PROTO_CODE_WARMUP;
    s_code_sent    = '\0';
    s_ppm          = 0.0f;
    s_ok_cycles    = 0u;
    s_fault_latched = false;
    s_meas_valid    = false;
    s_last_ok_ms    = 0u;
    s_last_sample_ms = 0u;
    s_last_tx_ms     = 0u;
    s_last_cmd_ms    = 0u;
    s_last_lcd_try_ms = 0u;
    s_link_down      = true;
    gas_level_init(&s_level);
    proto_rx_init(&s_rx);
}

void app_producer_task(void)
{
    handle_rx();

    if ((uint32_t)(millis() - s_last_cmd_ms) >= CFG_LINK_TIMEOUT_MS) {
        s_link_down = true;
    }

    if (elapsed(&s_last_sample_ms, CFG_SAMPLE_PERIOD_MS)) {
        advance_state();

        if (s_system_on && s_state != P_WARMUP) {
            sample_cycle();
        }

        refresh_code();

        send_telemetry();

        if (drv_lcd_absent()) {
            if (elapsed(&s_last_lcd_try_ms, CFG_LCD_RETRY_MS)) {
                if (drv_lcd_retry()) {
                    lcd_update();
                }
            }
        } else {
            lcd_update();
        }
    }

    if (s_code != s_code_sent) {
        send_status();
        s_last_tx_ms = millis();
    } else if (elapsed(&s_last_tx_ms, CFG_SAMPLE_PERIOD_MS)) {
        send_status();
    }
}

producer_state_t app_producer_state(void)
{
    return s_state;
}
