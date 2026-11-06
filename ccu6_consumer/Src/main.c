/**
 * @file    main.c
 * @brief   Diem vao cua Consumer: khoi tao, vong lap chinh, nap watchdog.
 */
#include "stm32f4xx.h"

#include "config.h"
#include "bsp_clock.h"
#include "bsp_gpio.h"
#include "bsp_systick.h"
#include "drv_uart.h"
#include "drv_tim.h"
#include "hal_output.h"
#include "hal_switch.h"
#include "drv_iwdg.h"
#include "app_consumer.h"

int main(void)
{
    bsp_clock_init();
    bsp_gpio_init();
    bsp_systick_init();

    hal_output_init();
    drv_uart_init();
    drv_tim_init();
    hal_switch_init();

    app_consumer_init();

    drv_iwdg_init();

    for (;;) {
        app_consumer_task();
        drv_iwdg_note_task_done();

        (void)drv_iwdg_service();

        __disable_irq();
        const bool idle = ((USART2->SR & USART_SR_RXNE) == 0u) &&
                          ((TIM3->SR & TIM_SR_UIF) == 0u) &&
                          ((EXTI->PR & (1u << CFG_SWITCH_PIN)) == 0u);
        if (idle) {
            __WFI();
        }
        __enable_irq();
    }
}
