#pragma once

#include <stdint.h>

/** Clamp an unsigned persisted setting to its supported inclusive range. */
uint8_t sc_settings_bound_u8(uint8_t value, uint8_t minimum, uint8_t maximum);
