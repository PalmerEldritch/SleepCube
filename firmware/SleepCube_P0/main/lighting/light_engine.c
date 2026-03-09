#include "light_engine.h"

#include <math.h>
#include "sdkconfig.h"

static float sc_clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static uint8_t sc_quantize_dither(float v, uint16_t frame_index, uint16_t channel_index)
{
    if (v <= 0.0f) {
        return 0;
    }
    if (v >= 255.0f) {
        return 255;
    }

    const uint8_t lo = (uint8_t)v;
    const uint8_t hi = (uint8_t)(lo + 1U);
    const float frac = v - (float)lo;
    const uint8_t threshold = (uint8_t)((frame_index + (channel_index * 73U)) & 0xFFU);
    return ((uint16_t)(frac * 255.0f) > threshold) ? hi : lo;
}

void sc_light_engine_render_warm(float brightness_pct, int8_t warmth_shift_pct, uint16_t frame_index,
                                 uint8_t *rgb_out, size_t led_count)
{
    const float clamped = sc_clampf(brightness_pct, 0.0f, 100.0f);
    const int8_t warmth = (int8_t)sc_clampf((float)warmth_shift_pct, -100.0f, 100.0f);

    // Perceptual -> electrical mapping. This smooths visible low-level stepping.
    const float perceptual = clamped / 100.0f;
    const float electrical = powf(perceptual, 2.2f);
    const float max_scale = ((float)CONFIG_SC_LED_MAX_BRIGHTNESS_PCT) / 100.0f;
    const float level = electrical * max_scale;

    const float base_r = ((float)CONFIG_SC_LIGHT_WARM_R) * level;
    const float base_g = ((float)CONFIG_SC_LIGHT_WARM_G) * level;
    const float base_b = ((float)CONFIG_SC_LIGHT_WARM_B) * level;

    const float warm_delta = (255.0f * level * (float)warmth) / 100.0f;
    const float rf = sc_clampf(base_r + warm_delta, 0.0f, 255.0f);
    const float gf = sc_clampf(base_g + (warm_delta / 3.0f), 0.0f, 255.0f);
    const float bf = sc_clampf(base_b - warm_delta, 0.0f, 255.0f);

    for (size_t i = 0; i < led_count; i++) {
        const uint16_t ch = (uint16_t)(i * 3U);
        rgb_out[(i * 3) + 0] = sc_quantize_dither(rf, frame_index, (uint16_t)(ch + 0U));
        rgb_out[(i * 3) + 1] = sc_quantize_dither(gf, frame_index, (uint16_t)(ch + 1U));
        rgb_out[(i * 3) + 2] = sc_quantize_dither(bf, frame_index, (uint16_t)(ch + 2U));
    }
}
