/**
 * @file    test_consumer.c
 * @brief   Kiem app_consumer.c THAT, chi gia lap ngoai vi va dong ho.
 */
#include "test_util.h"
#include "app_consumer.h"
#include "drv_uart.h"
#include "drv_tim.h"
#include "hal_output.h"
#include "hal_switch.h"
#include "bsp_systick.h"
#include "config.h"
static uint32_t now_ms;
static bool on=true, changed, gap, relay;
static const char *incoming="";
uint32_t millis(void) { return now_ms; }
uint32_t drv_tim_ticks(void) { return now_ms; }
bool hal_switch_is_on(void) { return on; }
bool hal_switch_poll(bool *out) { *out=on; bool event=changed; changed=false; return event; }
void hal_led_env(bool b,bool y,bool r) { (void)b;(void)y;(void)r; }
void hal_led_green(bool b) { (void)b; }
void hal_buzzer(bool b) { (void)b; }
void hal_relay_assert(void) { relay=true; }
void hal_relay_release(void) { relay=false; }
bool drv_uart_write(const char *s,int n) { (void)s;(void)n; return true; }
uart_rx_event_t drv_uart_read_event(char *c)
{
    if(gap) { gap=false; return UART_RX_GAP; }
    if(!*incoming) { return UART_RX_NONE; }
    *c=*incoming++; return UART_RX_BYTE;
}
static void step(uint32_t t,const char *rx) { now_ms=t; incoming=rx; app_consumer_task(); }
int main(void)
{
    SUITE("Real Consumer: known gap cannot downgrade alarm or refresh timeout");
    app_consumer_init(); CHECK(relay,"unknown producer at boot asserts relay");
    step(100u,"S3\n"); CHECK_EQ_I(app_consumer_state(),C_L3,"good L3");
    step(200u,"S"); gap=true; step(300u,"0\n");
    CHECK_EQ_I(app_consumer_state(),C_L3,"damaged frame does not become S0");
    CHECK(relay,"damaged frame does not release relay");
    step(100u+CFG_LINK_TIMEOUT_MS-1u,""); CHECK_EQ_I(app_consumer_state(),C_L3,"before timeout");
    step(100u+CFG_LINK_TIMEOUT_MS,""); CHECK_EQ_I(app_consumer_state(),C_LINK_LOST,"gap never refreshed last valid time");
    step(3200u,"S0\n"); CHECK_EQ_I(app_consumer_state(),C_L0,"clean frame recovers");
    CHECK(!relay,"real valid level zero follows configured output");
    on=false; changed=true; step(3300u,""); CHECK_EQ_I(app_consumer_state(),C_SYS_OFF,"switch off");
    on=true; changed=true; step(10000u,"");
    CHECK_EQ_I(app_consumer_state(),C_LINK_LOST,"F06 restart cannot revive stale S0");
    CHECK(relay,"restart without frame asserts relay");
    for(unsigned n=0u;n<=CFG_SYNC_TIMEOUT_MS/1000u;n++) { step(11000u+n*1000u,"SO\n"); }
    CHECK_EQ_I(app_consumer_state(),C_LINK_LOST,"F02 recurring SO cannot extend sync forever");
    return test_report("test_consumer");
}
