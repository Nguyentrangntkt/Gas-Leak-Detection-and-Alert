/**
 * @file    test_alarm_fsm.c
 * @brief   Kiem thu may trang thai canh bao va chinh sach dau ra.
 */
#include "test_util.h"
#include "alarm_fsm.h"
#include "protocol.h"
#include "config.h"

static void test_timeout(void)
{
    SUITE("Timeout UART");

    CHECK_EQ_I(alarm_resolve_state(true, '0', 0u, 0u),    C_L0, "moi nhan S0");
    CHECK_EQ_I(alarm_resolve_state(true, '2', 2999u, 0u), C_L2, "sat truoc timeout");

    CHECK_EQ_I(alarm_resolve_state(true, '0', 3000u, 0u), C_LINK_LOST,
               "dung bang timeout");
    CHECK_EQ_I(alarm_resolve_state(true, '0', 60000u, 0u), C_LINK_LOST,
               "qua han lau");
    CHECK_EQ_I(alarm_resolve_state(true, '3', 5000u, 0u), C_LINK_LOST,
               "mat ket noi ghi de ca muc 3");
}

static void test_priority(void)
{
    SUITE("Thu tu uu tien trang thai");

    CHECK_EQ_I(alarm_resolve_state(false, '3', 0u, 0u),     C_SYS_OFF, "tat + muc 3");
    CHECK_EQ_I(alarm_resolve_state(false, 'F', 99999u, 0u), C_SYS_OFF, "tat + loi");

    CHECK(alarm_resolve_state(false, '0', 0u, 0u) !=
          alarm_resolve_state(true, '0', 0u, 0u),
          "SYS_OFF khac muc 0");
}

static void test_code_mapping(void)
{
    SUITE("Anh xa ma trang thai");

    CHECK_EQ_I(alarm_resolve_state(true, '0', 0u, 0u), C_L0, "'0'");
    CHECK_EQ_I(alarm_resolve_state(true, '1', 0u, 0u), C_L1, "'1'");
    CHECK_EQ_I(alarm_resolve_state(true, '2', 0u, 0u), C_L2, "'2'");
    CHECK_EQ_I(alarm_resolve_state(true, '3', 0u, 0u), C_L3, "'3'");
    CHECK_EQ_I(alarm_resolve_state(true, 'W', 0u, 0u), C_WARMUP, "'W'");
    CHECK_EQ_I(alarm_resolve_state(true, 'F', 0u, 0u), C_SENSOR_FAULT, "'F'");
    CHECK_EQ_I(alarm_resolve_state(true, 'O', 0u, 0u), C_WARMUP,
               "'O' khi cong tac dang bat = chua dong bo");

    CHECK_EQ_I(alarm_resolve_state(true, 'Z', 0u, 0u), C_LINK_LOST, "ma la");
    CHECK_EQ_I(alarm_resolve_state(true, 0,   0u, 0u), C_LINK_LOST, "ma rong");

    CHECK_EQ_I(alarm_resolve_state(true, PROTO_CODE_NONE, 0u, 0u),
               C_LINK_LOST, "PROTO_CODE_NONE khong duoc thanh muc 0");
}

static void test_stop_mismatch_deadline(void)
{
    SUITE("Bat dong bo ON/'O' co deadline");

    CHECK_EQ_I(alarm_resolve_state(true, 'O', 0u, 0u), C_WARMUP,
               "vua bat, chua tinh bat dong bo");
    CHECK_EQ_I(alarm_resolve_state(true, 'O', 0u, 1u), C_WARMUP,
               "moi bat dau bat dong bo");
    CHECK_EQ_I(alarm_resolve_state(true, 'O', 0u, CFG_SYNC_TIMEOUT_MS - 1u),
               C_WARMUP, "sat truoc deadline van cho");
    CHECK_EQ_I(alarm_resolve_state(true, 'O', 0u, CFG_SYNC_TIMEOUT_MS),
               C_LINK_LOST, "dung deadline -> loi lien lac");
    CHECK_EQ_I(alarm_resolve_state(true, 'O', 0u, 600000u),
               C_LINK_LOST, "qua han lau -> van la loi, khong tro ve warm-up");

    alarm_outputs_t out;
    alarm_outputs(alarm_resolve_state(true, 'O', 0u, CFG_SYNC_TIMEOUT_MS),
                  true, true, &out);
    CHECK(out.relay_assert, "bat dong bo qua han phai kich relay");
    CHECK(out.buzzer, "bat dong bo qua han phai co canh bao am");

    CHECK_EQ_I(alarm_resolve_state(false, 'O', 0u, 600000u), C_SYS_OFF,
               "tat cong tac van uu tien truoc bat dong bo");

    CHECK_EQ_I(alarm_resolve_state(true, '2', 0u, 600000u), C_L2,
               "ma that khong bi deadline bat dong bo lam sai");
    CHECK_EQ_I(alarm_resolve_state(true, 'F', 0u, 600000u), C_SENSOR_FAULT,
               "loi cam bien giu nguyen y nghia");
}

static void test_outputs_safety(void)
{
    SUITE("Dau ra - rang buoc an toan");

    alarm_outputs_t o;

    alarm_outputs(C_SENSOR_FAULT, true, true, &o);
    CHECK(o.relay_assert, "loi cam bien -> kich relay");
    CHECK(!o.green, "loi cam bien -> khong sang LED he thong");

    alarm_outputs(C_LINK_LOST, true, true, &o);
    CHECK(o.relay_assert, "mat ket noi -> kich relay");

    alarm_outputs(C_L0, true, false, &o);
    CHECK(!o.relay_assert && !o.buzzer, "muc 0 - khong relay, khong coi");
    CHECK(o.blue && !o.yellow && !o.red, "muc 0 - LED xanh duong");

    alarm_outputs(C_L1, true, false, &o);
    CHECK(!o.relay_assert && !o.buzzer, "muc 1 - khong relay, khong coi");
    CHECK(!o.blue && o.yellow && !o.red, "muc 1 - LED vang");

    alarm_outputs(C_L2, true, false, &o);
    CHECK(o.relay_assert, "muc 2 -> kich relay");
    CHECK(o.buzzer, "muc 2 -> coi lien tuc");
    CHECK(o.red, "muc 2 -> LED do (pha sang)");

    alarm_outputs(C_L3, true, false, &o);
    CHECK(o.relay_assert && o.buzzer && o.red, "muc 3");

    alarm_outputs(C_L2, false, false, &o);
    CHECK(!o.red, "muc 2 - LED do tat o pha toi");
    CHECK(o.buzzer, "muc 2 - coi khong nhay theo LED");
    CHECK(o.relay_assert, "muc 2 - relay giu nguyen qua ca hai pha");

    alarm_outputs(C_SYS_OFF, true, true, &o);
    CHECK(!o.relay_assert, "tat he thong -> nha relay");
    CHECK(!o.buzzer, "tat he thong -> tat coi");
    CHECK(o.green, "tat he thong -> LED xanh la sang");
    CHECK(!o.blue && !o.yellow && !o.red, "tat he thong -> tat LED moi truong");

    alarm_outputs(C_SENSOR_FAULT, true, false, &o);
    CHECK(!o.buzzer, "loi cam bien - im o pha tat cua mau bip");
    alarm_outputs(C_L2, true, false, &o);
    CHECK(o.buzzer, "canh bao gas - coi khong phu thuoc pha bip");
}

static void test_blink_timing(void)
{
    SUITE("Tan so nhay LED");

    CHECK_EQ_I(alarm_blink_half_ms(C_L0, 2u), 0, "muc 0 khong nhay");
    CHECK_EQ_I(alarm_blink_half_ms(C_L1, 2u), 0, "muc 1 khong nhay");
    CHECK_EQ_I(alarm_blink_half_ms(C_SYS_OFF, 2u), 0, "tat he thong khong nhay");

    CHECK_EQ_I(alarm_blink_half_ms(C_L2, 2u), 500, "muc 2 = 1 Hz");

    CHECK_EQ_I(alarm_blink_half_ms(C_L3, 2u),  250, "muc 3 o 2 Hz");
    CHECK_EQ_I(alarm_blink_half_ms(C_L3, 5u),  100, "muc 3 o 5 Hz");
    CHECK_EQ_I(alarm_blink_half_ms(C_L3, 10u), 50,  "muc 3 o 10 Hz");

    for (uint32_t hz = CFG_BLINK_L3_HZ_MIN; hz <= CFG_BLINK_L3_HZ_MAX; hz++) {
        CHECK(alarm_blink_half_ms(C_L3, hz) < alarm_blink_half_ms(C_L2, hz),
              "muc 3 nhanh hon muc 2 tai %u Hz", (unsigned)hz);
    }

    CHECK(alarm_blink_half_ms(C_L3, 0u) > 0, "hz = 0 duoc chan");
}

static void test_blink_ramp(void)
{
    SUITE("Tang dan tan so o muc 3");

    uint32_t hz = CFG_BLINK_L3_HZ_MIN;
    for (uint32_t i = CFG_BLINK_L3_HZ_MIN; i < CFG_BLINK_L3_HZ_MAX; i++) {
        const uint32_t next = alarm_next_l3_hz(hz);
        CHECK(next == hz + 1u, "tang tu %u len %u", (unsigned)hz,
              (unsigned)next);
        hz = next;
    }
    CHECK_EQ_I(hz, CFG_BLINK_L3_HZ_MAX, "cham tran 10 Hz");
    CHECK_EQ_I(alarm_next_l3_hz(hz), CFG_BLINK_L3_HZ_MIN, "quay lai 2 Hz");

    hz = CFG_BLINK_L3_HZ_MIN;
    for (int i = 0; i < 200; i++) {
        hz = alarm_next_l3_hz(hz);
        CHECK(hz >= CFG_BLINK_L3_HZ_MIN && hz <= CFG_BLINK_L3_HZ_MAX,
              "hz trong khoang hop le");
    }
}

static void test_scenario(void)
{
    SUITE("Kich ban: boot -> warm-up -> leo thang -> mat UART -> tat");

    struct { bool on; char code; uint32_t age; consumer_state_t want;
             const char *what; } steps[] = {
        { true,  'W', 0u,     C_WARMUP,        "vua boot, dang lam nong" },
        { true,  '0', 0u,     C_L0,            "het lam nong, khong khi sach" },
        { true,  '1', 0u,     C_L1,            "canh bao som" },
        { true,  '2', 0u,     C_L2,            "canh bao cao" },
        { true,  '3', 0u,     C_L3,            "canh bao nghiem trong" },
        { true,  '3', 3500u,  C_LINK_LOST,     "rut day UART" },
        { true,  '1', 0u,     C_L1,            "cam lai day, nhan khung moi" },
        { true,  'F', 0u,     C_SENSOR_FAULT,  "Producer bao loi cam bien" },
        { false, 'F', 0u,     C_SYS_OFF,       "nguoi van hanh gat cong tac" },
        { true,  'O', 0u,     C_WARMUP,        "bat lai, Producer chua kip" },
        { true,  '0', 0u,     C_L0,            "dong bo xong" },
    };

    for (unsigned i = 0; i < sizeof(steps) / sizeof(steps[0]); i++) {
        const consumer_state_t got =
            alarm_resolve_state(steps[i].on, steps[i].code, steps[i].age, 0u);
        CHECK_EQ_I(got, steps[i].want, steps[i].what);
    }
}

int main(void)
{
    test_timeout();
    test_priority();
    test_code_mapping();
    test_stop_mismatch_deadline();
    test_outputs_safety();
    test_blink_timing();
    test_blink_ramp();
    test_scenario();
    return test_report("test_alarm_fsm");
}
