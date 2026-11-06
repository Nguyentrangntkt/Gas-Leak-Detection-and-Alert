/**
 * @file    test_util.h
 * @brief   Khung kiem thu toi gian, khong phu thuoc thu vien ngoai.
 */
#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static int g_pass = 0;
static int g_fail = 0;
static const char *g_suite = "";

#define SUITE(name)   do { g_suite = (name); \
    printf("\n--- %s ---\n", g_suite); } while (0)

#define CHECK(cond, fmt, ...)                                              \
    do {                                                                   \
        if (cond) {                                                        \
            g_pass++;                                                      \
        } else {                                                           \
            g_fail++;                                                      \
            printf("  FAIL %s:%d  " fmt "\n", __FILE__, __LINE__,          \
                   ##__VA_ARGS__);                                         \
        }                                                                  \
    } while (0)

#define CHECK_EQ_I(got, want, what)                                        \
    do {                                                                   \
        const long _g = (long)(got), _w = (long)(want);                    \
        CHECK(_g == _w, "%s: got %ld, muon %ld", what, _g, _w);            \
    } while (0)

#define CHECK_NEAR(got, want, tol, what)                                   \
    do {                                                                   \
        const double _g = (double)(got), _w = (double)(want);              \
        CHECK(fabs(_g - _w) <= (tol),                                      \
              "%s: got %.6f, muon %.6f (dung sai %.6f)", what, _g, _w,     \
              (double)(tol));                                              \
    } while (0)

#define CHECK_REL(got, want, rel, what)                                    \
    do {                                                                   \
        const double _g = (double)(got), _w = (double)(want);              \
        const double _d = (_w != 0.0) ? fabs((_g - _w) / _w) : fabs(_g);   \
        CHECK(_d <= (rel), "%s: got %.6f, muon %.6f (lech %.4f%%)",        \
              what, _g, _w, _d * 100.0);                                   \
    } while (0)

static int test_report(const char *title)
{
    printf("\n==== %s: %d dat, %d loi ====\n", title, g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}

#endif
