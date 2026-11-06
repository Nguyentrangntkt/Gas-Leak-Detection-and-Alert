/**
 * @file    test_i2c_bus.c
 * @brief   Kiem thuat toan giai phong bus I2C.
 */
#include "test_util.h"
#include "i2c_bus_clear.h"
#include "config.h"
typedef struct {
    bool scl, sda, scl_stuck, sda_stuck, frozen;
    unsigned pulses, release_after, calls, tick_calls;
    uint32_t time;
} bus_t;
static void drive(void *ctx, bool scl, bool sda)
{
    bus_t *b=ctx; b->calls++;
    if (!b->scl && scl && sda) {
        b->pulses++;
        if (b->release_after!=0u && b->pulses>=b->release_after) { b->sda_stuck=false; }
    }
    b->scl=scl; b->sda=sda;
}
static uint8_t read_lines(void *ctx)
{
    bus_t *b=ctx;
    return (uint8_t)((b->scl&&!b->scl_stuck ? 1u:0u) | (b->sda&&!b->sda_stuck ? 2u:0u));
}
static uint32_t now(void *ctx)
{
    bus_t *b=ctx;

    b->tick_calls++;
    if (!b->frozen && b->tick_calls%20u==0u) { b->time++; }
    return b->time;
}
static bool clear(bus_t *b, uint32_t ms, uint32_t polls)
{
    const i2c_bus_ops_t ops={ b, drive, read_lines, now };
    return i2c_bus_clear(&ops,ms,polls);
}
int main(void)
{
    SUITE("Bus-clear algorithm: release, permanent shorts, deadline, frozen tick");
    bus_t b={.scl=true,.sda=true};
    CHECK(clear(&b,CFG_I2C_RECOVERY_MS,CFG_I2C_RECOVERY_POLLS),"free lines: errata sequence");
    CHECK_EQ_I(b.pulses,0,"no recovery clocks on already free bus");
    for (unsigned n=1u;n<=9u;n++) {
        b=(bus_t){.scl=true,.sda=true,.sda_stuck=true,.release_after=n};
        CHECK(clear(&b,CFG_I2C_RECOVERY_MS,CFG_I2C_RECOVERY_POLLS),"slave releases after %u clocks",n);
        CHECK_EQ_I(b.pulses,n,"stop immediately when SDA released");
        CHECK(b.scl&&b.sda,"leave both output latches released");
    }
    b=(bus_t){.scl=true,.sda=true,.sda_stuck=true};
    CHECK(!clear(&b,30u,10000u),"permanent SDA low fails");
    CHECK_EQ_I(b.pulses,9,"never exceed nine pulses");
    CHECK(b.time<=30u,"shared time budget");
    b=(bus_t){.scl=true,.sda=true,.scl_stuck=true};
    CHECK(!clear(&b,30u,10000u),"SCL held low fails");
    CHECK_EQ_I(b.pulses,0,"do not clock a held SCL");

    bus_t small={.scl=true,.sda=true,.frozen=true};
    bus_t large={.scl=true,.sda=true,.frozen=true};
    CHECK(!clear(&small,30u,40u),"poll cap terminates when SysTick frozen");
    CHECK(!clear(&large,30u,400u),"larger cap terminates too");
    CHECK(large.tick_calls>small.tick_calls,
          "poll cap - not the clock - is what bounds the loop");
    CHECK(small.calls<10u && large.calls<10u,"no runaway line driving");
    b=(bus_t){.scl=true,.sda=true,.sda_stuck=true,.release_after=9u};
    CHECK(!clear(&b,2u,10000u),"deadline applies to whole sequence");
    b=(bus_t){.scl=true,.sda=true,.time=UINT32_MAX-2u};
    CHECK(clear(&b,30u,10000u),"time rollover");
    return test_report("test_i2c_bus");
}
