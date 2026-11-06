/**
 * @file    drv_lcd1602.h
 * @brief   LCD 16x2 HD44780 qua bo mo rong I2C PCF8574.
 */
#ifndef DRV_LCD1602_H
#define DRV_LCD1602_H

#include <stdint.h>
#include <stdbool.h>

bool drv_lcd_init(void);

bool drv_lcd_retry(void);

void drv_lcd_clear(void);
void drv_lcd_set_cursor(uint8_t row, uint8_t col);
void drv_lcd_print(const char *s);

void drv_lcd_print_line(uint8_t row, const char *s);

bool drv_lcd_absent(void);

#endif
