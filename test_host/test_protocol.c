/**
 * @file    test_protocol.c
 * @brief   Kiem thu dong/mo khung UART va kha nang tai dong bo.
 */
#include "test_util.h"
#include "protocol.h"
#include <string.h>

static int feed(proto_rx_t *rx, const char *s, char *last, uint8_t *last_len)
{
    int lines = 0;
    for (const char *p = s; *p != '\0'; p++) {
        if (proto_rx_push(rx, *p)) {
            lines++;
            memcpy(last, rx->buf, rx->len);
            *last_len = rx->len;
            proto_rx_init(rx);
        }
    }
    return lines;
}

static void test_build(void)
{
    SUITE("Dong khung");

    char buf[16];
    int n;

    n = proto_build_status(buf, (int)sizeof(buf), PROTO_CODE_L2);
    CHECK_EQ_I(n, 3, "do dai khung trang thai");
    CHECK(memcmp(buf, "S2\n", 3) == 0, "noi dung khung trang thai");

    n = proto_build_cmd(buf, (int)sizeof(buf), PROTO_CMD_START);
    CHECK_EQ_I(n, 2, "do dai lenh");
    CHECK(memcmp(buf, "I\n", 2) == 0, "noi dung lenh start");

    n = proto_build_telemetry(buf, (int)sizeof(buf), 1432u, PROTO_CODE_L1);
    CHECK_EQ_I(n, 9, "do dai telemetry");
    CHECK(memcmp(buf, "P01432,1\n", 9) == 0, "noi dung telemetry");

    n = proto_build_telemetry(buf, (int)sizeof(buf), 0u, PROTO_CODE_L0);
    CHECK(memcmp(buf, "P00000,0\n", 9) == 0, "telemetry 0 ppm");

    n = proto_build_telemetry(buf, (int)sizeof(buf), 123456u, PROTO_CODE_L3);
    CHECK(memcmp(buf, "P99999,3\n", 9) == 0, "telemetry bao hoa o 99999");

    CHECK_EQ_I(proto_build_status(buf, 2, PROTO_CODE_L0), 0, "cap qua nho");
    CHECK_EQ_I(proto_build_telemetry(buf, 8, 1u, PROTO_CODE_L0), 0,
               "cap telemetry qua nho");
}

static void test_parse_ok(void)
{
    SUITE("Mo khung hop le");

    proto_rx_t rx;
    char line[PROTO_MAX_LINE];
    uint8_t len = 0;
    char code = 0;

    proto_rx_init(&rx);
    CHECK_EQ_I(feed(&rx, "S0\n", line, &len), 1, "nhan duoc 1 dong");
    CHECK(proto_parse_status(line, len, &code), "phan tich S0");
    CHECK_EQ_I(code, PROTO_CODE_L0, "ma = 0");

    for (const char *s = "0123WFO"; *s != '\0'; s++) {
        char frame[4] = { 'S', *s, '\n', '\0' };
        proto_rx_init(&rx);
        CHECK_EQ_I(feed(&rx, frame, line, &len), 1, "khung hop le");
        CHECK(proto_parse_status(line, len, &code), "ma hop le duoc chap nhan");
    }

    proto_rx_init(&rx);
    CHECK_EQ_I(feed(&rx, "S3\r\n", line, &len), 1, "CRLF");
    CHECK(proto_parse_status(line, len, &code), "phan tich sau CR");
    CHECK_EQ_I(code, PROTO_CODE_L3, "ma = 3 sau CRLF");

    char cmd = 0;
    proto_rx_init(&rx);
    CHECK_EQ_I(feed(&rx, "I\n", line, &len), 1, "lenh I");
    CHECK(proto_parse_cmd(line, len, &cmd), "phan tich I");
    CHECK_EQ_I(cmd, PROTO_CMD_START, "lenh = start");

    proto_rx_init(&rx);
    CHECK_EQ_I(feed(&rx, "O\n", line, &len), 1, "lenh O");
    CHECK(proto_parse_cmd(line, len, &cmd), "phan tich O");
    CHECK_EQ_I(cmd, PROTO_CMD_STOP, "lenh = stop");
}

static void test_parse_bad(void)
{
    SUITE("Du lieu hong khong duoc tao ra trang thai sai");

    proto_rx_t rx;
    char line[PROTO_MAX_LINE];
    uint8_t len = 0;
    char code = 0;
    char cmd  = 0;

    proto_rx_init(&rx);
    (void)feed(&rx, "S9\n", line, &len);
    CHECK(!proto_parse_status(line, len, &code), "ma 9 bi tu choi");

    proto_rx_init(&rx);
    (void)feed(&rx, "SX\n", line, &len);
    CHECK(!proto_parse_status(line, len, &code), "ma X bi tu choi");

    proto_rx_init(&rx);
    (void)feed(&rx, "2\n", line, &len);
    CHECK(!proto_parse_status(line, len, &code), "thieu 'S'");

    proto_rx_init(&rx);
    CHECK_EQ_I(feed(&rx, "\n", line, &len), 0, "dong rong bi bo");
    CHECK_EQ_I(feed(&rx, "\n\n\n", line, &len), 0, "nhieu dong rong lien tiep");

    proto_rx_init(&rx);
    CHECK_EQ_I(feed(&rx, "SSSSSSSSSSSSSSSSSSSSSSSS\n", line, &len), 0,
               "dong tran bi bo");
    CHECK_EQ_I(feed(&rx, "S1\n", line, &len), 1, "tai dong bo sau khi tran");
    CHECK(proto_parse_status(line, len, &code), "dong sau khi tran van dung");
    CHECK_EQ_I(code, PROTO_CODE_L1, "ma = 1");

    proto_rx_init(&rx);
    (void)feed(&rx, "\x01\x02\xFF", line, &len);
    CHECK_EQ_I(feed(&rx, "\n", line, &len), 1, "dong rac van ket thuc");
    CHECK(!proto_parse_status(line, len, &code), "dong rac bi tu choi");
    CHECK_EQ_I(feed(&rx, "S2\n", line, &len), 1, "dong sau dong rac");
    CHECK(proto_parse_status(line, len, &code), "dong sau rac phan tich duoc");
    CHECK_EQ_I(code, PROTO_CODE_L2, "ma = 2");

    proto_rx_init(&rx);
    (void)feed(&rx, "S\n", line, &len);
    CHECK(!proto_parse_status(line, len, &code), "khung cut duoi bi tu choi");

    proto_rx_init(&rx);
    (void)feed(&rx, "X\n", line, &len);
    CHECK(!proto_parse_cmd(line, len, &cmd), "lenh la bi tu choi");

    proto_rx_init(&rx);
    (void)feed(&rx, "IO\n", line, &len);
    CHECK(!proto_parse_cmd(line, len, &cmd), "lenh 2 ky tu bi tu choi");

    proto_rx_init(&rx);
    (void)feed(&rx, "S2\n", line, &len);
    CHECK(!proto_parse_cmd(line, len, &cmd), "khung S khong phai lenh");

    proto_rx_init(&rx);
    (void)feed(&rx, "O\n", line, &len);
    CHECK(!proto_parse_status(line, len, &code), "lenh O khong phai khung S");
}

static void test_stream_resync(void)
{
    SUITE("Tai dong bo tren dong du lieu lien tuc");

    proto_rx_t rx;
    char line[PROTO_MAX_LINE];
    uint8_t len = 0;
    char code = 0;
    proto_rx_init(&rx);

    int good = 0;
    const char *stream = "S1\nS" "S3\n" "S0\n";
    for (const char *p = stream; *p != '\0'; p++) {
        if (proto_rx_push(&rx, *p)) {
            memcpy(line, rx.buf, rx.len);
            len = rx.len;
            proto_rx_init(&rx);
            if (proto_parse_status(line, len, &code)) {
                good++;
            }
        }
    }

    CHECK_EQ_I(good, 2, "so khung hop le sau khi mat byte");
    CHECK_EQ_I(code, PROTO_CODE_L0, "khung cuoi cung van dung");
}

static void test_code_to_level(void)
{
    SUITE("Doi ma trang thai sang muc");

    uint8_t lv = 99u;
    CHECK(proto_code_to_level('0', &lv) && lv == 0u, "'0' -> 0");
    CHECK(proto_code_to_level('1', &lv) && lv == 1u, "'1' -> 1");
    CHECK(proto_code_to_level('2', &lv) && lv == 2u, "'2' -> 2");
    CHECK(proto_code_to_level('3', &lv) && lv == 3u, "'3' -> 3");

    CHECK(!proto_code_to_level('W', &lv), "'W' khong phai muc gas");
    CHECK(!proto_code_to_level('F', &lv), "'F' khong phai muc gas");
    CHECK(!proto_code_to_level('O', &lv), "'O' khong phai muc gas");
    CHECK(!proto_code_to_level('4', &lv), "'4' khong hop le");
}

static void test_known_gap(void)
{
    SUITE("Known byte loss poisons entire line until fresh newline");
    const char *frames[] = { "S0\n", "S3\n", "I\n", "O\n" };
    for (unsigned f=0u; f<sizeof(frames)/sizeof(frames[0]); f++) {
        const char *frame=frames[f];
        for (unsigned pos=0u; pos<strlen(frame); pos++) {
            proto_rx_t rx; proto_rx_init(&rx);
            for (unsigned i=0u; i<pos; i++) { (void)proto_rx_push(&rx,frame[i]); }
            proto_rx_discard(&rx);
            bool completed=false;
            for (unsigned i=pos+1u; frame[i]; i++) {
                if (proto_rx_push(&rx,frame[i])) { completed=true; }
            }
            CHECK(!completed,"lost byte %u of frame %u cannot form a line",pos,f);
            (void)proto_rx_push(&rx,'\n');
            (void)proto_rx_push(&rx,'S'); (void)proto_rx_push(&rx,'2');
            CHECK(proto_rx_push(&rx,'\n'),"accept next clean line");
            char code;
            CHECK(proto_parse_status(rx.buf,rx.len,&code)&&code=='2',"exact resynced content");
        }
    }
    proto_rx_t rx; proto_rx_init(&rx);
    (void)proto_rx_push(&rx,'S'); proto_rx_discard(&rx);
    (void)proto_rx_push(&rx,'0'); proto_rx_discard(&rx);
    CHECK(!proto_rx_push(&rx,'\n'),"repeated gaps stay poisoned");
}

int main(void)
{
    test_build();
    test_parse_ok();
    test_parse_bad();
    test_stream_resync();
    test_code_to_level();
    test_known_gap();
    return test_report("test_protocol");
}
