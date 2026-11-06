/**
 * @file    drv_lcd1602.c
 * @brief   LCD 16x2 qua PCF8574, co cache dong va che do 'vang mat'.
 */
#include "drv_lcd1602.h"
#include "drv_i2c.h"
#include "bsp_systick.h"
#include "config.h"
#include <string.h>

#define LCD_RS      0x01u
#define LCD_EN      0x04u
#define LCD_BL      0x08u

_Static_assert(CFG_LCD_ROWS == 2u, "bang row_base chi co hai dong");

static bool s_absent = true;
static char s_lines[CFG_LCD_ROWS][CFG_LCD_COLS];
static bool s_line_valid[CFG_LCD_ROWS];

static void invalidate(void)
{
    for (uint8_t r = 0u; r < CFG_LCD_ROWS; r++) { s_line_valid[r] = false; }
}

static bool lcd_raw(uint8_t value)
{
    if (drv_i2c_write_byte(CFG_LCD_I2C_ADDR7, (uint8_t)(value | LCD_BL))) {
        return true;
    }
    s_absent = true;
    invalidate();
    return false;
}

static bool lcd_pulse(uint8_t value)
{
    return lcd_raw((uint8_t)(value | LCD_EN))
        && lcd_raw((uint8_t)(value & ~LCD_EN));
}

static bool lcd_write4(uint8_t nibble, uint8_t rs)
{
    return lcd_pulse((uint8_t)((nibble & 0xF0u) | rs));
}

static bool lcd_write8(uint8_t byte, uint8_t rs)
{
    return lcd_write4((uint8_t)(byte & 0xF0u), rs)
        && lcd_write4((uint8_t)(byte << 4), rs);
}

static bool lcd_cmd(uint8_t c)
{
    if (s_absent) {
        return false;
    }
    return lcd_write8(c, 0u);
}

static bool lcd_data(uint8_t c)
{
    if (s_absent) {
        return false;
    }
    return lcd_write8(c, LCD_RS);
}

bool drv_lcd_init(void)
{
    invalidate();

    s_absent = false;
    if (!lcd_raw(0x00u)) {
        return false;
    }

    delay_ms(50u);

    if (!lcd_write4(0x30u, 0u)) { return false; } delay_ms(5u);
    if (!lcd_write4(0x30u, 0u)) { return false; } delay_ms(1u);
    if (!lcd_write4(0x30u, 0u)) { return false; } delay_ms(1u);
    if (!lcd_write4(0x20u, 0u)) { return false; } delay_ms(1u);

    if (!lcd_cmd(0x28u)) { return false; }
    if (!lcd_cmd(0x0Cu)) { return false; }
    if (!lcd_cmd(0x06u)) { return false; }
    if (!lcd_cmd(0x01u)) { return false; }
    delay_ms(2u);

    return true;
}

bool drv_lcd_retry(void)
{
    if (!s_absent) {
        return true;
    }
    return drv_lcd_init();
}

void drv_lcd_clear(void)
{
    invalidate();
    if (lcd_cmd(0x01u)) {
        delay_ms(2u);
    }
}

void drv_lcd_set_cursor(uint8_t row, uint8_t col)
{
    static const uint8_t row_base[CFG_LCD_ROWS] = { 0x00u, 0x40u };

    if (row >= CFG_LCD_ROWS) {
        row = (uint8_t)(CFG_LCD_ROWS - 1u);
    }
    if (col >= CFG_LCD_COLS) {
        col = (uint8_t)(CFG_LCD_COLS - 1u);
    }
    (void)lcd_cmd((uint8_t)(0x80u | (row_base[row] + col)));
}

void drv_lcd_print(const char *s)
{
    invalidate();
    while (*s != '\0') {
        if (!lcd_data((uint8_t)*s++)) {
            return;
        }
    }
}

void drv_lcd_print_line(uint8_t row, const char *s)
{
    if (s_absent) { return; }
    if (row >= CFG_LCD_ROWS) { row = (uint8_t)(CFG_LCD_ROWS - 1u); }
    char line[CFG_LCD_COLS];
    bool ended = false;
    for (uint8_t i = 0u; i < CFG_LCD_COLS; i++) {
        if (!ended && s[i] == '\0') { ended = true; }
        line[i] = ended ? ' ' : s[i];
    }
    if (s_line_valid[row] && memcmp(s_lines[row], line, CFG_LCD_COLS) == 0) {
        return;
    }
    drv_lcd_set_cursor(row, 0u);
    for (uint8_t i = 0u; i < CFG_LCD_COLS; i++) {
        if (!lcd_data((uint8_t)line[i])) { return; }
    }
    memcpy(s_lines[row], line, CFG_LCD_COLS);
    s_line_valid[row] = true;
}

bool drv_lcd_absent(void)
{
    return s_absent;
}
