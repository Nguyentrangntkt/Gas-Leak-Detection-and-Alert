/**
 * @file    test_buzzer_config.c
 * @brief   Kiem alarm_outputs() khi dich voi nguong coi = 3.
 */
#include "test_util.h"
#include "alarm_fsm.h"
#include "config.h"
int main(void)
{
    SUITE("Actual alarm_outputs built with buzzer threshold 3");
    alarm_outputs_t o;
    CHECK_EQ_I(CFG_BUZZER_FROM_LEVEL, 3, "test variant really overrides config");
    alarm_outputs(C_L2, true, true, &o);
    CHECK(!o.buzzer && o.relay_assert, "L2 silent but relay still asserted");
    alarm_outputs(C_L3, false, false, &o);
    CHECK(o.buzzer && o.relay_assert, "L3 continuous alarm");
    alarm_outputs(C_SENSOR_FAULT, true, true, &o);
    CHECK(o.buzzer && o.relay_assert, "fault beep independent of level threshold");
    alarm_outputs(C_SENSOR_FAULT, true, false, &o);
    CHECK(!o.buzzer, "fault silent phase");
    alarm_outputs(C_LINK_LOST, false, true, &o);
    CHECK(o.buzzer && o.relay_assert, "link fault independent of threshold");
    alarm_outputs(C_LINK_LOST, false, false, &o);
    CHECK(!o.buzzer, "link silent phase");
    return test_report("test_buzzer_config");
}
