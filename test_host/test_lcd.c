/**
 * @file    test_lcd.c
 * @brief   Kiem driver LCD that: cache dong, dem khoang trang, loi giua dong.
 */
#include "test_util.h"
#include "drv_lcd1602.h"
#include "drv_i2c.h"
#include "config.h"

#define ROW_WRITES (4u + 4u * CFG_LCD_COLS)
static unsigned writes, fail_at;
bool drv_i2c_write_byte(uint8_t addr, uint8_t value)
{
    (void)addr; (void)value; writes++;
    return fail_at==0u || writes!=fail_at;
}
void delay_ms(uint32_t ms) { (void)ms; }
int main(void)
{
    SUITE("Real LCD driver: row cache, padding, invalidation and mid-row failure");
    CHECK(drv_lcd_init(),"init"); writes=0u;
    drv_lcd_print_line(0u,"ABC"); CHECK_EQ_I(writes,ROW_WRITES,"cursor + mot dong ky tu");
    drv_lcd_print_line(0u,"ABC             "); CHECK_EQ_I(writes,ROW_WRITES,"normalized row cached");
    drv_lcd_print_line(1u,"second"); CHECK_EQ_I(writes,2u*ROW_WRITES,"other row first draw");
    drv_lcd_print_line(1u,"second"); CHECK_EQ_I(writes,2u*ROW_WRITES,"unchanged second row skipped");
    drv_lcd_print_line(0u,"AB"); CHECK_EQ_I(writes,3u*ROW_WRITES,"shorter content redraws trailing spaces");
    fail_at=writes+10u;
    drv_lcd_print_line(0u,"changed"); CHECK(drv_lcd_absent(),"mid-row NACK marks absent");
    unsigned stopped=writes;
    drv_lcd_print_line(1u,"second"); CHECK_EQ_I(writes,stopped,"absent is no-op");
    fail_at=0u; CHECK(drv_lcd_retry(),"reinit after failure"); writes=0u;
    drv_lcd_print_line(1u,"second"); CHECK_EQ_I(writes,ROW_WRITES,"reinit invalidates even unchanged row");
    drv_lcd_print_line(0u,"changed"); CHECK_EQ_I(writes,2u*ROW_WRITES,"failed row never treated as cached");
    drv_lcd_clear(); writes=0u;
    drv_lcd_print_line(0u,"changed"); CHECK_EQ_I(writes,ROW_WRITES,"clear invalidates cache");
    drv_lcd_print("raw"); writes=0u;
    drv_lcd_print_line(0u,"changed"); CHECK_EQ_I(writes,ROW_WRITES,"arbitrary writes invalidate cache");
    fail_at=writes+1u; drv_lcd_print_line(0u,"cursor fails");
    CHECK_EQ_I(writes,ROW_WRITES+1u,"cursor NACK stops before all data writes");
    return test_report("test_lcd");
}
