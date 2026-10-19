#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SC_LIGHT_PWM_DUTY_BPS_MAX (10000U)

bool sc_light_pwm_limits_valid(uint8_t minimum_duty_pct, uint8_t maximum_duty_pct);

/**
 * @brief Map logical brightness to PWM duty in basis points (0..10000).
 *
 * Logical 0 is always electrically off. Values from 1..100 map linearly from
 * the configured minimum to maximum duty. Fractional logical values below 1
 * are reserved for smooth startup/shutdown ramps and map from 0 to the minimum
 * duty without a discontinuity.
 */
uint16_t sc_light_pwm_map_brightness(float logical_brightness_pct,
                                     uint8_t minimum_duty_pct,
                                     uint8_t maximum_duty_pct);
