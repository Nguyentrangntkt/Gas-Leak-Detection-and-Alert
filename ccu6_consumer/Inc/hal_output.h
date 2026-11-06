/**
 * @file    hal_output.h
 * @brief   Dieu khien LED, buzzer, tin hieu relay.
 */
#ifndef HAL_OUTPUT_H
#define HAL_OUTPUT_H

#include <stdint.h>
#include <stdbool.h>

void hal_output_init(void);

void hal_led_env(bool blue, bool yellow, bool red);

void hal_led_green(bool on);

void hal_buzzer(bool on);

void hal_relay_assert(void);
void hal_relay_release(void);
bool hal_relay_is_asserted(void);

void hal_output_all_off(void);

#endif
