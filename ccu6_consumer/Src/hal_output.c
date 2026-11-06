/**
 * @file    hal_output.c
 * @brief   Dieu khien LED, buzzer va tin hieu relay.
 */
#include "stm32f4xx.h"
#include "hal_output.h"
#include "config.h"

static bool s_relay_asserted = false;

static void pin_write(GPIO_TypeDef *port, uint32_t pin, bool level)
{
    port->BSRR = level ? (1u << pin) : (1u << (pin + 16u));
}

void hal_output_init(void)
{
    hal_output_all_off();
}

void hal_led_env(bool blue, bool yellow, bool red)
{
#if CFG_LED_ACTIVE_HIGH
    pin_write(GPIOB, CFG_LED_PORT_BLUE_PIN,   blue);
    pin_write(GPIOB, CFG_LED_PORT_YELLOW_PIN, yellow);
    pin_write(GPIOB, CFG_LED_PORT_RED_PIN,    red);
#else
    pin_write(GPIOB, CFG_LED_PORT_BLUE_PIN,   !blue);
    pin_write(GPIOB, CFG_LED_PORT_YELLOW_PIN, !yellow);
    pin_write(GPIOB, CFG_LED_PORT_RED_PIN,    !red);
#endif
}

void hal_led_green(bool on)
{
#if CFG_LED_ACTIVE_HIGH
    pin_write(GPIOB, CFG_LED_PORT_GREEN_PIN, on);
#else
    pin_write(GPIOB, CFG_LED_PORT_GREEN_PIN, !on);
#endif
}

void hal_buzzer(bool on)
{
#if CFG_BUZZER_ACTIVE_HIGH
    pin_write(GPIOA, CFG_BUZZER_PIN, on);
#else
    pin_write(GPIOA, CFG_BUZZER_PIN, !on);
#endif
}

void hal_relay_assert(void)
{
#if CFG_RELAY_ACTIVE_LOW
    pin_write(GPIOB, CFG_RELAY_PIN, false);
#else
    pin_write(GPIOB, CFG_RELAY_PIN, true);
#endif
    s_relay_asserted = true;
}

void hal_relay_release(void)
{
#if CFG_RELAY_ACTIVE_LOW
    pin_write(GPIOB, CFG_RELAY_PIN, true);
#else
    pin_write(GPIOB, CFG_RELAY_PIN, false);
#endif
    s_relay_asserted = false;
}

bool hal_relay_is_asserted(void)
{
    return s_relay_asserted;
}

void hal_output_all_off(void)
{
    hal_led_env(false, false, false);
    hal_led_green(false);
    hal_buzzer(false);
    hal_relay_release();
}
