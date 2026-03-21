#include "light_engine.h"

#include <math.h>
#include "sdkconfig.h"

#ifdef CONFIG_SC_LIGHT_SPATIAL_UNDULATION_ENABLE
#define SC_LIGHT_SPATIAL_UNDULATION_ENABLE 1
#else
#define SC_LIGHT_SPATIAL_UNDULATION_ENABLE 0
#endif

#ifdef CONFIG_SC_LIGHT_SPATIAL_UNDULATION_PCT
#define SC_LIGHT_SPATIAL_UNDULATION_PCT CONFIG_SC_LIGHT_SPATIAL_UNDULATION_PCT
#else
#define SC_LIGHT_SPATIAL_UNDULATION_PCT 0
#endif

#ifdef CONFIG_SC_LIGHT_SPATIAL_KNOTS
#define SC_LIGHT_SPATIAL_KNOTS CONFIG_SC_LIGHT_SPATIAL_KNOTS
#else
#define SC_LIGHT_SPATIAL_KNOTS 2
#endif

#ifdef CONFIG_SC_LIGHT_SPATIAL_SPEED_PCT
#define SC_LIGHT_SPATIAL_SPEED_PCT CONFIG_SC_LIGHT_SPATIAL_SPEED_PCT
#else
#define SC_LIGHT_SPATIAL_SPEED_PCT 100
#endif

#define SC_LIGHT_MAX_SPATIAL_KNOTS 8
#define SC_TWO_PI (6.28318530718f)

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

static float sc_lerp(float a, float b, float t)
{
    return a + ((b - a) * t);
}

static float sc_spatial_knot_wave(float time_s, uint8_t knot_idx)
{
    const float speed = (float)SC_LIGHT_SPATIAL_SPEED_PCT / 100.0f;
    const float h = (float)(((uint32_t)knot_idx * 97U + 31U) % 251U) / 250.0f;
    const float f1_hz = (0.018f + (0.030f * h)) * speed;
    const float f2_hz = (0.009f + (0.020f * (1.0f - h))) * speed;
    const float p1 = SC_TWO_PI * (0.17f + (0.71f * h));
    const float p2 = SC_TWO_PI * (0.49f + (0.33f * h));
    const float a = sinf((SC_TWO_PI * f1_hz * time_s) + p1);
    const float b = sinf((SC_TWO_PI * f2_hz * time_s) + p2);
    return (0.62f * a) + (0.38f * b);
}

void sc_light_engine_render_warm(float brightness_pct, int8_t warmth_shift_pct,
                                 float time_s, uint16_t frame_index,
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

#if SC_LIGHT_SPATIAL_UNDULATION_ENABLE
    float knots[SC_LIGHT_MAX_SPATIAL_KNOTS];
    int knot_count = SC_LIGHT_SPATIAL_KNOTS;
    if (knot_count < 2) {
        knot_count = 2;
    }
    if (knot_count > SC_LIGHT_MAX_SPATIAL_KNOTS) {
        knot_count = SC_LIGHT_MAX_SPATIAL_KNOTS;
    }
    for (int k = 0; k < knot_count; k++) {
        knots[k] = sc_spatial_knot_wave(time_s, (uint8_t)k);
    }
    const float spatial_amp = (float)SC_LIGHT_SPATIAL_UNDULATION_PCT / 100.0f;
#endif

    for (size_t i = 0; i < led_count; i++) {
        float local_scale = 1.0f;
#if SC_LIGHT_SPATIAL_UNDULATION_ENABLE
        if (led_count > 1) {
            const float x = ((float)i * (float)(knot_count - 1)) / (float)(led_count - 1);
            int i0 = (int)x;
            if (i0 < 0) {
                i0 = 0;
            }
            int i1 = i0 + 1;
            if (i1 >= knot_count) {
                i1 = knot_count - 1;
            }
            const float t = x - (float)i0;
            const float undulation = sc_lerp(knots[i0], knots[i1], t);
            local_scale = sc_clampf(1.0f + (spatial_amp * undulation), 0.55f, 1.45f);
        }
#endif

        const float lr = sc_clampf(rf * local_scale, 0.0f, 255.0f);
        const float lg = sc_clampf(gf * local_scale, 0.0f, 255.0f);
        const float lb = sc_clampf(bf * local_scale, 0.0f, 255.0f);
        const uint16_t ch = (uint16_t)(i * 3U);
        rgb_out[(i * 3) + 0] = sc_quantize_dither(lr, frame_index, (uint16_t)(ch + 0U));
        rgb_out[(i * 3) + 1] = sc_quantize_dither(lg, frame_index, (uint16_t)(ch + 1U));
        rgb_out[(i * 3) + 2] = sc_quantize_dither(lb, frame_index, (uint16_t)(ch + 2U));
    }
}
