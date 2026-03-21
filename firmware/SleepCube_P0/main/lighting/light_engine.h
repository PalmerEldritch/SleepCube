#pragma once

#include <stddef.h>
#include <stdint.h>

void sc_light_engine_render_warm(float brightness_pct, int8_t warmth_shift_pct,
                                 float time_s, uint16_t frame_index,
                                 uint8_t *rgb_out, size_t led_count);
