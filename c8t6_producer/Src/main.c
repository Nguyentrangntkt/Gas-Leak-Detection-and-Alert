/**
 * @file    main.c
 * @brief   Diem vao cua Producer: khoi tao ngoai vi roi chay vong lap chinh.
 */
#include "stm32f1xx.h"

#include "config.h"
#include "bsp_clock.h"
#include "bsp_gpio.h"
#include "bsp_systick.h"
#include "drv_adc.h"
#include "drv_uart.h"
#include "drv_i2c.h"
#include "drv_lcd1602.h"
#include "app_producer.h"

int main(void)
{
    bsp_clock_init();
    bsp_systick_init();
    bsp_gpio_init();

    drv_uart_init();
    (void)drv_adc_init();
    drv_i2c_init();
    (void)drv_lcd_init();

    app_producer_init();

    for (;;) {
        app_producer_task();
        __WFI();
    }
}
