/**
 * @file    test_ring.c
 * @brief   Kiem thu so hoc ring buffer va quy tac ghi tron khung.
 */
#include "test_util.h"
#include "ring.h"
#include "config.h"

#include <string.h>

#define RING_SZ 8u

static void test_used_room_empty(void)
{
    SUITE("Ring rong");

    CHECK_EQ_I(ring_used(0u, 0u, RING_SZ), 0, "used khi head=tail=0");
    CHECK_EQ_I(ring_room(0u, 0u, RING_SZ), RING_SZ - 1u, "room khi rong");

    for (uint16_t i = 0u; i < RING_SZ; i++) {
        CHECK_EQ_I(ring_used(i, i, RING_SZ), 0, "used khi head=tail");
        CHECK_EQ_I(ring_room(i, i, RING_SZ), RING_SZ - 1u, "room khi head=tail");
    }
}

static void test_used_room_general(void)
{
    SUITE("used + room = size - 1 voi moi cap chi so");

    for (uint16_t h = 0u; h < RING_SZ; h++) {
        for (uint16_t t = 0u; t < RING_SZ; t++) {
            const uint32_t used = ring_used(h, t, RING_SZ);
            const uint32_t room = ring_room(h, t, RING_SZ);
            CHECK(used <= RING_SZ - 1u, "used khong vuot size-1 (h=%u t=%u)",
                  (unsigned)h, (unsigned)t);
            CHECK_EQ_I(used + room, RING_SZ - 1u, "used + room");
        }
    }
}

static void test_wrap(void)
{
    SUITE("Quay vong");

    CHECK_EQ_I(ring_used(1u, 6u, RING_SZ), 3, "used khi head<tail");
    CHECK_EQ_I(ring_room(1u, 6u, RING_SZ), 4, "room khi head<tail");

    CHECK_EQ_I(ring_used(5u, 6u, RING_SZ), RING_SZ - 1u, "used khi day");
    CHECK_EQ_I(ring_room(5u, 6u, RING_SZ), 0, "room khi day");

    CHECK_EQ_I(ring_next(RING_SZ - 1u, RING_SZ), 0, "next vong ve 0");
    CHECK_EQ_I(ring_next(0u, RING_SZ), 1, "next tu 0");
}

typedef struct {
    char     buf[RING_SZ];
    uint16_t head;
    uint16_t tail;
    unsigned dropped;
} fake_ring_t;

static int fake_write(fake_ring_t *r, const char *data, int len)
{
    if (!ring_write_frame(r->buf, &r->head, &r->tail, RING_SZ, data, len)) {
        r->dropped++;
        return 0;
    }
    return 1;
}

static int fake_pop(fake_ring_t *r, char *out)
{
    return ring_read_byte(r->buf, &r->head, &r->tail, RING_SZ, out);
}

static void test_all_or_nothing(void)
{
    SUITE("Khung tron ven hoac bi bo han");

    fake_ring_t r;
    memset(&r, 0, sizeof(r));

    CHECK(fake_write(&r, "S1\n", 3) == 1, "khung 1 vao duoc");
    CHECK(fake_write(&r, "S2\n", 3) == 1, "khung 2 vao duoc");

    CHECK(fake_write(&r, "S3\n", 3) == 0, "khung 3 bi bo vi khong du cho");
    CHECK_EQ_I(r.dropped, 1, "dem khung bi bo");
    CHECK_EQ_I(ring_used(r.head, r.tail, RING_SZ), 6,
               "khong co byte nao cua khung bi bo lot vao");

    char out[16];
    int n = 0;
    while (fake_pop(&r, &out[n])) {
        n++;
    }
    out[n] = '\0';
    CHECK_EQ_I(n, 6, "so byte doc ra");
    CHECK(strcmp(out, "S1\nS2\n") == 0, "noi dung doc ra: '%s'", out);

    CHECK(fake_write(&r, "S3\n", 3) == 1, "khung vao duoc sau khi xa");
}

static void test_never_truncates(void)
{
    SUITE("Khong bao gio xuat hien khung thieu byte");

    for (uint16_t start = 0u; start < RING_SZ; start++) {
        fake_ring_t r;
        memset(&r, 0, sizeof(r));
        r.head = start;
        r.tail = start;

        int written = 0;
        for (int k = 0; k < 10; k++) {
            written += fake_write(&r, "P1\n", 3) ? 3 : 0;
        }

        char c;
        int n = 0;
        while (fake_pop(&r, &c)) {
            n++;
        }
        CHECK_EQ_I(n, written, "doc ra dung so byte da ghi");
        CHECK_EQ_I(n % 3, 0, "so byte luon la boi so cua do dai khung");
    }
}

static void test_uart_ring_sizes(void)
{
    SUITE("Kich thuoc ring thuc te du cho khung dai nhat");

    const uint16_t tx_ring = CFG_UART_TX_RING;
    CHECK(ring_room(0u, 0u, tx_ring) >= 9u,
          "ring TX rong chua duoc khung 9 byte: room = %u",
          (unsigned)ring_room(0u, 0u, tx_ring));
}

int main(void)
{
    test_used_room_empty();
    test_used_room_general();
    test_wrap();
    test_all_or_nothing();
    test_never_truncates();
    test_uart_ring_sizes();
    return test_report("test_ring");
}
