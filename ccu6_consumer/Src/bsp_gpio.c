/**
 * @file    bsp_gpio.c
 * @brief   Cau hinh chan cho Consumer. Dua chan relay ve muc an toan truoc tien.
 */
#include "stm32f4xx.h"
#include "bsp_gpio.h"
#include "config.h"

static void set_moder(GPIO_TypeDef *port, uint32_t pin, uint32_t mode)
{
    port->MODER = (port->MODER & ~(3u << (pin * 2u))) | (mode << (pin * 2u));
}

static void set_pupd(GPIO_TypeDef *port, uint32_t pin, uint32_t pupd)
{
    port->PUPDR = (port->PUPDR & ~(3u << (pin * 2u))) | (pupd << (pin * 2u));
}

static void set_af(GPIO_TypeDef *port, uint32_t pin, uint32_t af)
{
    const uint32_t idx = pin >> 3u;
    const uint32_t sh  = (pin & 7u) * 4u;
    port->AFR[idx] = (port->AFR[idx] & ~(0xFu << sh)) | (af << sh);
}

void bsp_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                    RCC_AHB1ENR_GPIOCEN;

#if CFG_RELAY_ACTIVE_LOW
    GPIOB->BSRR = (1u << CFG_RELAY_PIN);
#else
    GPIOB->BSRR = (1u << (CFG_RELAY_PIN + 16u));
#endif
    set_moder(GPIOB, CFG_RELAY_PIN, 1u);

#if CFG_BUZZER_ACTIVE_HIGH
    GPIOA->BSRR = (1u << (CFG_BUZZER_PIN + 16u));
#else
    GPIOA->BSRR = (1u << CFG_BUZZER_PIN);
#endif
    set_moder(GPIOA, CFG_BUZZER_PIN, 1u);

    GPIOB->BSRR = (1u << (CFG_LED_PORT_BLUE_PIN   + 16u))
                | (1u << (CFG_LED_PORT_YELLOW_PIN + 16u))
                | (1u << (CFG_LED_PORT_RED_PIN    + 16u))
                | (1u << (CFG_LED_PORT_GREEN_PIN  + 16u));
    set_moder(GPIOB, CFG_LED_PORT_BLUE_PIN,   1u);
    set_moder(GPIOB, CFG_LED_PORT_YELLOW_PIN, 1u);
    set_moder(GPIOB, CFG_LED_PORT_RED_PIN,    1u);

    set_pupd(GPIOB,  CFG_LED_PORT_GREEN_PIN,  0u);
    set_moder(GPIOB, CFG_LED_PORT_GREEN_PIN,  1u);

    set_moder(GPIOC, CFG_SWITCH_PIN, 0u);
    set_pupd(GPIOC,  CFG_SWITCH_PIN, 1u);

    set_af(GPIOA, 2u, 7u);
    set_af(GPIOA, 3u, 7u);
    set_moder(GPIOA, 2u, 2u);
    set_moder(GPIOA, 3u, 2u);
    set_pupd(GPIOA,  3u, 1u);
}
