#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "light_pwm_policy.h"

static void assert_near_u16(uint16_t actual, uint16_t expected, uint16_t tolerance)
{
    const uint16_t difference = (actual > expected) ? (actual - expected) : (expected - actual);
    assert(difference <= tolerance);
}

int main(void)
{
    assert(sc_light_pwm_limits_valid(5U, 80U));
    assert(sc_light_pwm_limits_valid(0U, 100U));
    assert(!sc_light_pwm_limits_valid(81U, 80U));
    assert(!sc_light_pwm_limits_valid(5U, 101U));

    assert(sc_light_pwm_map_brightness(0.0f, 5U, 80U) == 0U);
    /* Negative effects saturate at the fully-off lower limit. */
    assert(sc_light_pwm_map_brightness(-10.0f, 5U, 80U) == 0U);
    assert(sc_light_pwm_map_brightness(NAN, 5U, 80U) == 0U);
    assert(sc_light_pwm_map_brightness(1.0f, 5U, 80U) == 500U);
    assert_near_u16(sc_light_pwm_map_brightness(50.0f, 5U, 80U), 4212U, 1U);
    assert(sc_light_pwm_map_brightness(100.0f, 5U, 80U) == 8000U);
    /* Positive effects saturate at the configured 80% upper limit. */
    assert(sc_light_pwm_map_brightness(120.0f, 5U, 80U) == 8000U);

    /* Startup/shutdown values remain continuous below the active 5% floor. */
    assert(sc_light_pwm_map_brightness(0.25f, 5U, 80U) == 125U);
    assert(sc_light_pwm_map_brightness(0.5f, 5U, 80U) == 250U);
    assert(sc_light_pwm_map_brightness(0.75f, 5U, 80U) == 375U);

    /* Invalid limits fail off instead of producing an unsafe duty. */
    assert(sc_light_pwm_map_brightness(50.0f, 81U, 80U) == 0U);

    puts("light PWM policy tests passed");
    return 0;
}
