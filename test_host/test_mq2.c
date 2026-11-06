/**
 * @file    test_mq2.c
 * @brief   Kiem thu quy doi ppm, loc nhieu, phan muc va tuoi phep do.
 */
#include "test_util.h"
#include "mq2_sensor.h"
#include "config.h"

static void test_adc_to_mv(void)
{
    SUITE("ADC 12-bit -> dien ap");

    CHECK_NEAR(mq2_adc_to_mv(0u),    0.0f,    0.001f, "raw=0");
    CHECK_NEAR(mq2_adc_to_mv(4095u), 3300.0f, 0.001f, "raw=4095 (het thang)");
    CHECK_NEAR(mq2_adc_to_mv(2048u), 1650.403f, 0.01f, "raw=2048");

    const float wrong = (2048.0f / 1023.0f) * 3300.0f;
    CHECK(wrong > 6000.0f, "minh hoa loi 10-bit: %.1f mV", (double)wrong);
}

static void test_rs(void)
{
    SUITE("Dien ap -> Rs");

    float rs = 0.0f;

    CHECK_EQ_I(mq2_rs_from_adc(0u, &rs),    MQ2_ERR_ADC_LOW,  "raw=0 -> loi");
    CHECK_EQ_I(mq2_rs_from_adc(19u, &rs),   MQ2_ERR_ADC_LOW,  "raw=19 -> loi");
    CHECK_EQ_I(mq2_rs_from_adc(4090u, &rs), MQ2_ERR_ADC_HIGH, "raw=4090 -> loi");
    CHECK_EQ_I(mq2_rs_from_adc(4095u, &rs), MQ2_ERR_ADC_HIGH, "raw=4095 -> loi");

    CHECK_EQ_I(mq2_rs_from_adc(20u, &rs), MQ2_OK, "raw=20 van hop le");
    CHECK(rs > 0.0f, "Rs duong o raw=20: %.1f", (double)rs);

    CHECK_EQ_I(mq2_rs_from_adc(2048u, &rs), MQ2_OK, "raw=2048 hop le");
    CHECK_REL(rs, 999.515, 0.001, "Rs tai raw=2048");

    CHECK_EQ_I(mq2_rs_from_adc(2146u, &rs), MQ2_OK, "raw=2146 hop le");
    CHECK_REL(rs, 908.29, 0.002, "Rs trong khong khi sach");
    CHECK_REL(rs / CFG_MQ2_R0_OHM, 9.5, 0.002, "ty so trong khong khi sach");
}

static void test_curve_equivalence(void)
{
    SUITE("Hai dang duong dac tuyen phai tuong duong");

    for (double r = 0.1; r <= 20.0; r *= 1.35) {
        const float a = mq2_ppm_from_ratio_pow((float)r);
        const float b = mq2_ppm_from_ratio_log((float)r);
        CHECK_REL(b, a, 0.002, "dang log vs dang luy thua");
    }

    const double r = 0.7797;
    const double right = 574.25 * pow(r, -2.222);
    const double wrong = pow(10.0, (log10(r) - 1.2577) / -0.450045);
    CHECK(fabs(wrong - right) / right > 0.05,
          "b sai 1.2577 gay lech %.1f%%",
          100.0 * fabs(wrong - right) / right);
}

static void test_ppm_reference_points(void)
{
    SUITE("Diem tham chieu cua duong dac tuyen");

    CHECK(mq2_ppm_from_ratio(9.5f) < 10.0f,
          "khong khi sach: %.2f ppm", (double)mq2_ppm_from_ratio(9.5f));

    CHECK_REL(mq2_ppm_from_ratio(1.0f), 574.25, 0.001, "r=1");

    CHECK_REL(mq2_ppm_from_ratio(0.779116f), 1000.0, 0.01, "r -> 1000 ppm");
    CHECK_REL(mq2_ppm_from_ratio(0.649150f), 1500.0, 0.01, "r -> 1500 ppm");
    CHECK_REL(mq2_ppm_from_ratio(0.570258f), 2000.0, 0.01, "r -> 2000 ppm");

    float prev = mq2_ppm_from_ratio(0.3f);
    for (float r = 0.4f; r < 10.0f; r += 0.3f) {
        const float cur = mq2_ppm_from_ratio(r);
        CHECK(cur < prev, "don dieu giam tai r=%.2f", (double)r);
        prev = cur;
    }
}

static void test_r0_calibration(void)
{
    SUITE("Hieu chuan R0 doc lap");

    CHECK_REL(mq2_r0_from_rs_air(908.2f), 95.6, 0.001, "R0 tu Rs khong khi");
    CHECK_REL(mq2_r0_from_rs_air(CFG_MQ2_R0_OHM * 9.5f), CFG_MQ2_R0_OHM,
              0.001, "nhat quan hai chieu");
}

static void test_filter(void)
{
    SUITE("Loc 3-sigma + trung vi");

    float a[21];
    for (int i = 0; i < 20; i++) {
        a[i] = 100.0f;
    }
    a[20] = 5000.0f;
    CHECK_NEAR(mq2_filter_rs(a, 21), 100.0f, 0.001f,
               "nhieu dot bien bi loai");

    float b[5] = { 1.0f, 2.0f, 3.0f, 4.0f, 100.0f };
    CHECK_NEAR(mq2_filter_rs(b, 5), 3.0f, 0.001f,
               "trung vi gan chiu duoc ngoai lai con sot");

    float c[1] = { 42.0f };
    CHECK_NEAR(mq2_filter_rs(c, 1), 42.0f, 0.001f, "mot mau");

    float d[4] = { 10.0f, 10.0f, 10.0f, 10.0f };
    CHECK_NEAR(mq2_filter_rs(d, 4), 10.0f, 0.001f, "sigma = 0");
}

static void test_median_even_count(void)
{
    SUITE("Trung vi khi so mau chan");

    float a[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
    CHECK_NEAR(mq2_filter_rs(a, 4), 20.0f, 0.001f,
               "lay trung vi duoi, khong phai tren");

    CHECK(mq2_filter_rs((float[]){ 10.0f, 20.0f, 30.0f, 40.0f }, 4) != 25.0f,
          "khong tao gia tri moi ngoai tap mau");

    float b[5] = { 10.0f, 20.0f, 30.0f, 40.0f, 50.0f };
    CHECK_NEAR(mq2_filter_rs(b, 5), 30.0f, 0.001f, "so mau le khong doi");

    float c[2] = { 7.0f, 9.0f };
    CHECK_NEAR(mq2_filter_rs(c, 2), 7.0f, 0.001f, "hai mau -> lay cai nho");

    float e[20];
    for (int i = 0; i < 10; i++) {
        e[i]      = 908.0f;
        e[10 + i] = 50.0f;
    }
    const float med = mq2_filter_rs(e, 20);
    CHECK_NEAR(med, 50.0f, 0.001f,
               "hai nhom bang nhau -> chon ben Rs thap (ppm cao)");
    CHECK(mq2_ppm_from_ratio(med / CFG_MQ2_R0_OHM) >= CFG_GAS_TH3_PPM,
          "ket qua phai la muc canh bao cao, khong phai muc 0");
}

static void test_process(void)
{
    SUITE("Chuoi day du: ADC -> ppm");

    uint16_t raw[CFG_SAMPLE_COUNT];
    float    scratch[CFG_SAMPLE_COUNT];
    mq2_result_t res;

    for (int i = 0; i < CFG_SAMPLE_COUNT; i++) {
        raw[i] = 2146u;
    }
    CHECK_EQ_I(mq2_process(raw, CFG_SAMPLE_COUNT, scratch, &res), MQ2_OK,
               "khong khi sach hop le");
    CHECK(res.ppm < 10.0f, "khong khi sach: %.2f ppm", (double)res.ppm);
    CHECK_EQ_I(gas_level_raw(res.ppm), 0, "khong khi sach -> muc 0");

    for (int i = 0; i < CFG_SAMPLE_COUNT; i++) {
        raw[i] = 3811u;
    }
    CHECK_EQ_I(mq2_process(raw, CFG_SAMPLE_COUNT, scratch, &res), MQ2_OK,
               "raw=3811 hop le");
    CHECK_REL(res.ppm, 1000.0, 0.05, "raw=3811 -> ~1000 ppm");

    for (int i = 0; i < CFG_SAMPLE_COUNT; i++) {
        raw[i] = 3811u;
    }
    raw[3]  = 0u;
    raw[7]  = 4095u;
    CHECK_EQ_I(mq2_process(raw, CFG_SAMPLE_COUNT, scratch, &res), MQ2_OK,
               "chiu duoc vai mau hong");
    CHECK_REL(res.ppm, 1000.0, 0.05, "ket qua van dung sau khi loai mau hong");

    for (int i = 0; i < CFG_SAMPLE_COUNT; i++) {
        raw[i] = 0u;
    }
    CHECK(mq2_process(raw, CFG_SAMPLE_COUNT, scratch, &res) != MQ2_OK,
          "toan mau hong -> bao loi cam bien");

    for (int i = 0; i < CFG_SAMPLE_COUNT; i++) {
        raw[i] = 4095u;
    }
    CHECK(mq2_process(raw, CFG_SAMPLE_COUNT, scratch, &res) != MQ2_OK,
          "ngan mach len Vcc -> bao loi cam bien");
}

static void test_levels(void)
{
    SUITE("Phan muc canh bao 1000 / 1500 / 2000 ppm");

    CHECK_EQ_I(gas_level_raw(0.0f),     0, "0 ppm");
    CHECK_EQ_I(gas_level_raw(999.9f),   0, "999.9 ppm");
    CHECK_EQ_I(gas_level_raw(1000.0f),  1, "1000 ppm (bien duoi muc 1)");
    CHECK_EQ_I(gas_level_raw(1499.9f),  1, "1499.9 ppm");
    CHECK_EQ_I(gas_level_raw(1500.0f),  2, "1500 ppm");
    CHECK_EQ_I(gas_level_raw(1999.9f),  2, "1999.9 ppm");
    CHECK_EQ_I(gas_level_raw(2000.0f),  3, "2000 ppm");
    CHECK_EQ_I(gas_level_raw(50000.0f), 3, "rat cao");

    CHECK(CFG_GAS_TH1_PPM < CFG_GAS_TH2_PPM &&
          CFG_GAS_TH2_PPM < CFG_GAS_TH3_PPM, "nguong tang dan");
    CHECK(CFG_GAS_TH2_PPM - CFG_GAS_HYST2_PPM > CFG_GAS_TH1_PPM,
          "hysteresis khong chong lan nguong duoi");
    CHECK(CFG_GAS_TH3_PPM - CFG_GAS_HYST3_PPM > CFG_GAS_TH2_PPM,
          "hysteresis muc 3 khong chong lan muc 2");
}

static void test_hysteresis(void)
{
    SUITE("Hysteresis va chong dao dong trang thai");

    gas_level_t st;
    gas_level_init(&st);
    CHECK_EQ_I(st.level, 0, "khoi tao o muc 0");

    CHECK_EQ_I(gas_level_update(&st, 1010.0f), 1, "len muc 1 ngay lap tuc");
    CHECK_EQ_I(gas_level_update(&st, 2500.0f), 3, "nhay thang len muc 3");

    CHECK_EQ_I(gas_level_update(&st, 1000.0f), 3, "chua ha o chu ky 1");
    CHECK_EQ_I(gas_level_update(&st, 1000.0f), 3, "chua ha o chu ky 2");
    CHECK_EQ_I(gas_level_update(&st, 1000.0f), 1, "ha xuong muc 1 o chu ky 3");

    gas_level_init(&st);
    (void)gas_level_update(&st, 1010.0f);
    for (int i = 0; i < 50; i++) {
        const float ppm = (i % 2 == 0) ? 990.0f : 1010.0f;
        CHECK_EQ_I(gas_level_update(&st, ppm), 1,
                   "dao dong quanh nguong van giu muc 1");
    }

    CHECK_EQ_I(gas_level_update(&st, 850.0f), 1, "chu ky 1 duoi vung tru");
    CHECK_EQ_I(gas_level_update(&st, 850.0f), 1, "chu ky 2");
    CHECK_EQ_I(gas_level_update(&st, 850.0f), 0, "chu ky 3 -> ve muc 0");

    gas_level_init(&st);
    (void)gas_level_update(&st, 1600.0f);
    CHECK_EQ_I(gas_level_update(&st, 800.0f),  2, "dem 1");
    CHECK_EQ_I(gas_level_update(&st, 1600.0f), 2, "quay lai -> reset dem");
    CHECK_EQ_I(gas_level_update(&st, 800.0f),  2, "dem lai tu dau: 1");
    CHECK_EQ_I(gas_level_update(&st, 800.0f),  2, "dem 2");
    CHECK_EQ_I(gas_level_update(&st, 800.0f),  0, "dem 3 -> ve muc 0");
}

static void test_downgrade_chain_broken_by_invalid(void)
{
    SUITE("Chu ky loi ngat chuoi ha cap");

    gas_level_t st;
    gas_level_init(&st);
    (void)gas_level_update(&st, 2500.0f);
    CHECK_EQ_I(st.level, 3, "dang o muc 3");

    CHECK_EQ_I(gas_level_update(&st, 800.0f), 3, "thap lan 1");
    CHECK_EQ_I(gas_level_update(&st, 800.0f), 3, "thap lan 2");
    gas_level_invalidate(&st);
    CHECK_EQ_I(st.downgrade_count, 0, "chu ky loi xoa bo dem");
    CHECK_EQ_I(gas_level_update(&st, 800.0f), 3,
               "sau chu ky loi phai dem lai tu dau, KHONG ha cap");
    CHECK_EQ_I(gas_level_update(&st, 800.0f), 3, "thap lan 2 cua chuoi moi");
    CHECK_EQ_I(gas_level_update(&st, 800.0f), 0,
               "du ba chu ky thap lien tiep thi moi ha");

    gas_level_init(&st);
    (void)gas_level_update(&st, 1600.0f);
    for (int i = 0; i < 20; i++) {
        gas_level_invalidate(&st);
        CHECK_EQ_I(st.level, 2, "thieu du lieu khong duoc ha muc");
    }
}

static void test_measurement_freshness(void)
{
    SUITE("Tuoi phep do");

    const uint32_t max_age = CFG_MEAS_MAX_AGE_MS;

    CHECK(!meas_is_fresh(false, 0u, 0u, max_age),
          "chua tung do -> khong duoc cong bo muc");
    CHECK(!meas_is_fresh(false, 1000000u, 999999u, max_age),
          "chua tung do, moc thoi gian khong lam no hop le");

    CHECK(meas_is_fresh(true, 1000u, 1000u, max_age), "vua do xong");
    CHECK(meas_is_fresh(true, 1000u + max_age, 1000u, max_age),
          "dung bang han muc van con hop le");
    CHECK(!meas_is_fresh(true, 1000u + max_age + 1u, 1000u, max_age),
          "qua han mot ms la het hop le");

    CHECK(meas_is_fresh(true, 2000u, 0u, max_age),
          "hai chu ky loi lien tiep van duoc che bang so do truoc");
    CHECK(!meas_is_fresh(true, 3000u, 0u, max_age),
          "chu ky loi thu ba thi phai bao loi");

    const uint32_t near_wrap = 0xFFFFFF00u;
    CHECK(meas_is_fresh(true, near_wrap + 100u, near_wrap, max_age),
          "qua moc tran: con trong han");
    CHECK(!meas_is_fresh(true, near_wrap + max_age + 1u, near_wrap, max_age),
          "qua moc tran: het han van phat hien dung");
}

int main(void)
{
    test_adc_to_mv();
    test_rs();
    test_curve_equivalence();
    test_ppm_reference_points();
    test_r0_calibration();
    test_filter();
    test_median_even_count();
    test_process();
    test_levels();
    test_hysteresis();
    test_downgrade_chain_broken_by_invalid();
    test_measurement_freshness();
    return test_report("test_mq2");
}
