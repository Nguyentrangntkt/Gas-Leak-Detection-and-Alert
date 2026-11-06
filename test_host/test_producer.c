/**
 * @file    test_producer.c
 * @brief   Kiem app_producer.c THAT, chi gia lap ngoai vi va dong ho.
 */
#include "test_util.h"
#include "app_producer.h"
#include "drv_uart.h"
#include "drv_adc.h"
#include "drv_lcd1602.h"
#include "bsp_systick.h"
#include "timebase.h"
#include "bsp_clock.h"
#include "config.h"
#include <string.h>
static uint32_t now_ms, adc_reads;
static uint16_t adc_raw;
static bool adc_ok, gap;
static const char *incoming;
static char status;
static char transcript[1024];
static unsigned sent;
uint32_t millis(void) { return now_ms; }
void delay_ms(uint32_t ms) { now_ms += ms; }

bool elapsed(uint32_t *last, uint32_t period)
{
    return time_elapsed(now_ms, last, period);
}
bool bsp_clock_degraded(void) { return false; }
bool bsp_clock_confirmed(void) { return true; }
bool drv_adc_read(uint16_t *v) { adc_reads++; *v=adc_raw; return adc_ok; }
uart_rx_event_t drv_uart_read_event(uart_id_t id, char *v)
{
    (void)id;
    if (gap) { gap=false; return UART_RX_GAP; }
    if (!*incoming) { return UART_RX_NONE; }
    *v=*incoming++; return UART_RX_BYTE;
}
bool drv_uart_write(uart_id_t id, const char *v, int n)
{
    if (id==UART_LINK && n==3) {
        status=v[1];
        if (sent+3u < sizeof(transcript)) {
            memcpy(&transcript[sent], v, 3u); sent+=3u; transcript[sent]='\0';
        }
    }
    return true;
}
bool drv_lcd_absent(void) { return false; }
bool drv_lcd_retry(void) { return true; }
void drv_lcd_print_line(uint8_t r, const char *s) { (void)r; (void)s; }
static void reset(uint16_t raw)
{
    now_ms=0u; adc_reads=0u; adc_raw=raw; adc_ok=true; gap=false;
    incoming=""; status='?'; sent=0u; transcript[0]='\0'; app_producer_init();
}
static void step(uint32_t t) { now_ms=t; app_producer_task(); }
static void command(const char *s, uint32_t t) { incoming=s; step(t); }
static void test_initial_invalid(void)
{
    SUITE("F01: first invalid measurement cannot emit S0");
    reset(0u); step(1000u); CHECK_EQ_I(status,'W',"warmup");
    CHECK_EQ_I(adc_reads,0,"no ADC while warming");
    step(CFG_WARMUP_MS); CHECK_EQ_I(status,'F',"invalid first cycle faults immediately");
    CHECK(strstr(transcript,"S0\n")==NULL,"no fabricated valid measurement");
    reset(2146u); adc_ok=false; step(CFG_WARMUP_MS);
    CHECK_EQ_I(status,'F',"ADC EOC timeout follows same fault path");
}
static void test_recovery_stop_start(void)
{
    SUITE("F08/F09: Stop/Start preserves fault latch and replies immediately");
    reset(0u); step(30000u); CHECK_EQ_I(status,'F',"latched fault");
    command("O\n",31000u); CHECK_EQ_I(status,'O',"stop");
    unsigned before=adc_reads;
    command("I\n",31100u);
    CHECK_EQ_I(status,'F',"immediate start reports retained fault");
    CHECK_EQ_I(adc_reads,before,"RX start performs no ADC batch");
    CHECK_EQ_I(now_ms,31100u,"RX start has no sample delays");
    adc_raw=2146u;
    for (unsigned n=1u;n<CFG_SENSOR_FAULT_CYCLES;n++) {
        step(31000u+n*1000u);
        CHECK_EQ_I(status,'F',"fewer than five good cycles remain fault");
    }
    step(31000u+CFG_SENSOR_FAULT_CYCLES*1000u);
    CHECK(app_producer_state()==P_RUN && status!='F',"fifth good cycle clears latch");
    reset(0u); step(30000u); adc_raw=2146u; step(31000u); step(32000u);
    adc_ok=false; step(33000u); adc_ok=true;
    for (unsigned n=0u;n<4u;n++) { step(34000u+n*1000u); CHECK_EQ_I(status,'F',"bad cycle resets good streak"); }
    step(38000u); CHECK_EQ_I(app_producer_state(),P_RUN,"five consecutive good cycles recover");
}
static void test_stale_start(void)
{
    SUITE("F09: stale Start returns W before new measurement, never old O/0");
    reset(3900u); step(30000u); CHECK_EQ_I(status,'3',"real mq2 model reaches L3");
    command("O\n",31000u); step(40000u);
    unsigned before=adc_reads;
    command("I\n",40100u);
    CHECK_EQ_I(status,'W',"stale data -> not ready");
    CHECK_EQ_I(adc_reads,before,"no sample on command path");
    step(41000u); CHECK_EQ_I(status,'3',"next measured cycle reports gas");
}
static void test_invalid_breaks_downgrade(void)
{
    SUITE("F07: invalid cycle interrupts downgrade confirmation");
    reset(3900u); step(30000u);
    adc_raw=2146u; step(31000u); step(32000u);
    adc_raw=0u; step(33000u); adc_raw=2146u; step(34000u);
    CHECK_EQ_I(status,'3',"low-low-invalid-low cannot release L3");
    step(35000u); CHECK_EQ_I(status,'3',"only two new low cycles");
    step(36000u); CHECK(status!='3',"three new low cycles can downgrade");
}
static void test_gap_command(void)
{
    SUITE("F11: real app discards command at known RX gap");
    reset(3900u); step(30000u);
    command("O",30300u); CHECK_EQ_I(app_producer_state(),P_RUN,"partial stop");
    gap=true; command("\n",30400u);
    CHECK_EQ_I(app_producer_state(),P_RUN,"gap prevents fabricated stop");
    command("O\n",30500u); CHECK_EQ_I(status,'O',"new clean stop accepted");
    gap=true; command("I\n",30600u); CHECK_EQ_I(status,'O',"first post-gap line discarded");
    command("I\n",30700u); CHECK_EQ_I(status,'3',"subsequent clean start uses fresh measurement");
}

static void test_time_elapsed(void)
{
    SUITE("time_elapsed: nguong, cap nhat moc, va moc tran 32 bit");
    uint32_t last = 1000u;
    CHECK(!time_elapsed(1000u, &last, 1000u), "cung moc thi chua du");
    CHECK_EQ_I(last, 1000u, "that bai thi KHONG duoc cap nhat moc");
    CHECK(!time_elapsed(1999u, &last, 1000u), "thieu 1 ms");
    CHECK_EQ_I(last, 1000u, "van giu moc cu");
    CHECK(time_elapsed(2000u, &last, 1000u), "dung bang nguong la du");
    CHECK_EQ_I(last, 2000u, "thanh cong thi cap nhat moc");
    CHECK(!time_elapsed(2001u, &last, 1000u), "moc moi tinh lai tu dau");

    last = 5u;
    CHECK(time_elapsed(5u, &last, 0u), "period 0 luon qua");

    last = 0xFFFFFF00u;
    CHECK(!time_elapsed(0xFFFFFF00u + 999u, &last, 1000u),
          "qua moc tran: chua du thi van bao chua du");
    CHECK(time_elapsed(0xFFFFFF00u + 1000u, &last, 1000u),
          "qua moc tran: du thi phai nhan ra");
    CHECK_EQ_I(last, (uint32_t)(0xFFFFFF00u + 1000u), "moc moi sau khi tran");

    last = 0u;
    CHECK(time_elapsed(0x7FFFFFFFu, &last, 0x7FFFFFFFu), "nua dai 32 bit");
}

int main(void)
{
    test_initial_invalid(); test_recovery_stop_start(); test_stale_start();
    test_invalid_breaks_downgrade(); test_gap_command(); test_time_elapsed();
    return test_report("test_producer");
}
