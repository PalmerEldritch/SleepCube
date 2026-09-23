#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "settings_policy.h"

int main(void)
{
    assert(sc_settings_bound_u8(0U, 5U, 100U) == 5U);
    assert(sc_settings_bound_u8(5U, 5U, 100U) == 5U);
    assert(sc_settings_bound_u8(37U, 5U, 100U) == 37U);
    assert(sc_settings_bound_u8(100U, 5U, 100U) == 100U);
    assert(sc_settings_bound_u8(UINT8_MAX, 5U, 100U) == 100U);
    assert(sc_settings_bound_u8(42U, 100U, 5U) == 100U);
    puts("settings policy tests passed");
    return 0;
}
