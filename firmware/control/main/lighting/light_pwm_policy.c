#include "light_pwm_policy.h"

#include <math.h>

bool sc_light_pwm_limits_valid(uint8_t minimum_duty_pct, uint8_t maximum_duty_pct)
{
    return (minimum_duty_pct <= maximum_duty_pct) && (maximum_duty_pct <= 100U);
}

uint16_t sc_light_pwm_map_brightness(float logical_brightness_pct,
                                     uint8_t minimum_duty_pct,
                                     uint8_t maximum_duty_pct)
{
    if (!sc_light_pwm_limits_valid(minimum_duty_pct, maximum_duty_pct) ||
        !isfinite(logical_brightness_pct) || (logical_brightness_pct <= 0.0f)) {
        return 0U;
    }

    float logical = logical_brightness_pct;
    if (logical > 100.0f) {
        logical = 100.0f;
    }

    float duty_pct;
    if (logical < 1.0f) {
        duty_pct = logical * (float)minimum_duty_pct;
    } else {
        const float span = (float)maximum_duty_pct - (float)minimum_duty_pct;
        duty_pct = (float)minimum_duty_pct + (((logical - 1.0f) / 99.0f) * span);
    }

    long duty_bps = lroundf(duty_pct * 100.0f);
    if (duty_bps < 0L) {
        return 0U;
    }
    if (duty_bps > (long)SC_LIGHT_PWM_DUTY_BPS_MAX) {
        return SC_LIGHT_PWM_DUTY_BPS_MAX;
    }
    return (uint16_t)duty_bps;
}
