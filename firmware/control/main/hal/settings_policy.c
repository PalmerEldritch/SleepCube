#include "settings_policy.h"

uint8_t sc_settings_bound_u8(uint8_t value, uint8_t minimum, uint8_t maximum)
{
    if (minimum > maximum) {
        return minimum;
    }
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}
