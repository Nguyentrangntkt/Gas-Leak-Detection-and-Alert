/**
 * @file    test_debounce.c
 * @brief   Kiem thu bo chong rung cong tac.
 */
#include "test_util.h"
#include "debounce.h"
#include "config.h"

#define NEED  CFG_DEBOUNCE_MS

static void feed(debounce_t *d, bool raw, unsigned n)
{
    for (unsigned i = 0u; i < n; i++) {
        debounce_tick(d, raw);
    }
}

static void test_init(void)
{
    SUITE("Khoi tao theo muc dang doc duoc");

    debounce_t d;
    debounce_init(&d, false, NEED);
    CHECK(!debounce_level(&d), "khoi tao muc 0");

    bool out = true;
    CHECK(!debounce_poll(&d, &out), "khong co su kien ngay sau khoi tao");

    debounce_init(&d, true, NEED);
    CHECK(debounce_level(&d), "khoi tao muc 1");
    CHECK(!debounce_poll(&d, &out), "cung khong co su kien");
}

static void test_needs_full_window(void)
{
    SUITE("Phai on dinh du cua so moi duoc chot");

    debounce_t d;
    debounce_init(&d, false, NEED);

    bool out = false;

    feed(&d, true, NEED - 1u);
    CHECK(!debounce_poll(&d, &out), "thieu 1 ms -> chua co su kien");
    CHECK(!debounce_level(&d), "muc on dinh van la cu");

    feed(&d, true, 1u);
    CHECK(debounce_poll(&d, &out), "du cua so -> co su kien");
    CHECK(out, "muc moi la 1");
    CHECK(debounce_level(&d), "muc on dinh da doi");

    feed(&d, true, 1000u);
    CHECK(!debounce_poll(&d, &out), "khong co su kien lap lai");
}

static void test_bounce_rejected(void)
{
    SUITE("Rung khong tao ra su kien");

    debounce_t d;
    debounce_init(&d, false, NEED);
    bool out = false;

    unsigned events = 0u;
    for (unsigned i = 0u; i < 500u; i++) {
        debounce_tick(&d, (i % 2u) == 0u);
        if (debounce_poll(&d, &out)) {
            events++;
        }
    }
    CHECK_EQ_I(events, 0, "rung lien tuc khong tao su kien nao");
    CHECK(!debounce_level(&d), "muc on dinh giu nguyen gia tri ban dau");

    for (unsigned i = 0u; i < 20u; i++) {
        debounce_tick(&d, (i % 2u) == 0u);
    }
    feed(&d, true, NEED);
    CHECK(debounce_poll(&d, &out) && out, "on dinh sau rung -> mot su kien");
    CHECK(!debounce_poll(&d, &out), "khong co su kien thu hai");
}

static void test_bounce_back_to_old_level(void)
{
    SUITE("Rung roi tro ve muc cu");

    debounce_t d;
    debounce_init(&d, false, NEED);
    bool out = false;

    feed(&d, true, NEED - 2u);
    feed(&d, false, NEED);
    CHECK(!debounce_poll(&d, &out), "xung ngan khong tao su kien");
    CHECK(!debounce_level(&d), "muc on dinh van la 0");
}

static void tick_then_poll_interleaved(void)
{
    SUITE("Suon moi chen vao giua poll khong lam sai ket qua");

    debounce_t d;
    debounce_init(&d, false, NEED);

    bool out   = false;
    bool level = false;

    feed(&d, true, NEED);
    if (debounce_poll(&d, &out)) {
        level = out;
    }
    CHECK(level, "da nhan muc 1");

    unsigned bad = 0u;
    for (unsigned i = 0u; i < 400u; i++) {
        debounce_tick(&d, (i % 3u) == 0u);
        if (debounce_poll(&d, &out)) {
            level = out;

            if (level != debounce_level(&d)) {
                bad++;
            }
        }
    }
    CHECK_EQ_I(bad, 0, "khong su kien nao bao mot muc chua duoc chot");

    feed(&d, false, NEED);
    CHECK(debounce_poll(&d, &out), "on dinh ve 0 -> co su kien");
    CHECK(!out, "muc bao ra la 0");
    CHECK(!debounce_level(&d), "muc on dinh la 0");
}

static void test_level_matches_last_event(void)
{
    SUITE("debounce_level luon khop su kien gan nhat");

    debounce_t d;
    debounce_init(&d, false, NEED);
    bool out      = false;
    bool reported = false;

    for (unsigned round = 0u; round < 10u; round++) {
        const bool target = ((round % 2u) == 0u);
        feed(&d, target, NEED);
        if (debounce_poll(&d, &out)) {
            reported = out;
        }
        CHECK(reported == debounce_level(&d),
              "muc da bao khop muc on dinh o vong %u", round);
    }
}

int main(void)
{
    test_init();
    test_needs_full_window();
    test_bounce_rejected();
    test_bounce_back_to_old_level();
    tick_then_poll_interleaved();
    test_level_matches_last_event();
    return test_report("test_debounce");
}
