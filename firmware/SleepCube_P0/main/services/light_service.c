#include "light_service.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "settings_store.h"

#if CONFIG_SC_ENABLE_LIGHT
#include "esp_random.h"
#include "led_strip_if.h"
#include "light_engine.h"
#include "sc_trace.h"
#endif

static const char *TAG = "sc_light_service";

#if CONFIG_SC_ENABLE_LIGHT

#ifdef CONFIG_SC_LIGHT_DEFAULT_BRIGHTNESS_PCT
#define SC_LIGHT_DEFAULT_BRIGHTNESS_PCT CONFIG_SC_LIGHT_DEFAULT_BRIGHTNESS_PCT
#else
#define SC_LIGHT_DEFAULT_BRIGHTNESS_PCT 30
#endif

#ifdef CONFIG_SC_LIGHT_UPDATE_HZ
#define SC_LIGHT_UPDATE_HZ CONFIG_SC_LIGHT_UPDATE_HZ
#else
#define SC_LIGHT_UPDATE_HZ 50
#endif

#ifdef CONFIG_SC_LIGHT_RAMP_STEP_PCT
#define SC_LIGHT_RAMP_STEP_PCT CONFIG_SC_LIGHT_RAMP_STEP_PCT
#else
#define SC_LIGHT_RAMP_STEP_PCT 2
#endif

#ifdef CONFIG_SC_LIGHT_STARTUP_RAMP_MS
#define SC_LIGHT_STARTUP_RAMP_MS CONFIG_SC_LIGHT_STARTUP_RAMP_MS
#else
#define SC_LIGHT_STARTUP_RAMP_MS 3000
#endif

#ifdef CONFIG_SC_LIGHT_STARTUP_OVERSHOOT_PCT
#define SC_LIGHT_STARTUP_OVERSHOOT_PCT CONFIG_SC_LIGHT_STARTUP_OVERSHOOT_PCT
#else
#define SC_LIGHT_STARTUP_OVERSHOOT_PCT 6
#endif

#ifdef CONFIG_SC_LIGHT_FLUCT_ENABLE
#define SC_LIGHT_FLUCT_ENABLE 1
#else
#define SC_LIGHT_FLUCT_ENABLE 0
#endif

#ifdef CONFIG_SC_LIGHT_FLUCT_BRIGHTNESS_PCT
#define SC_LIGHT_FLUCT_BRIGHTNESS_PCT CONFIG_SC_LIGHT_FLUCT_BRIGHTNESS_PCT
#else
#define SC_LIGHT_FLUCT_BRIGHTNESS_PCT 0
#endif

#ifdef CONFIG_SC_LIGHT_FLUCT_WARMTH_PCT
#define SC_LIGHT_FLUCT_WARMTH_PCT CONFIG_SC_LIGHT_FLUCT_WARMTH_PCT
#else
#define SC_LIGHT_FLUCT_WARMTH_PCT 0
#endif

#ifdef CONFIG_SC_LIGHT_AMBIENT_SPEED_PCT
#define SC_LIGHT_AMBIENT_SPEED_PCT CONFIG_SC_LIGHT_AMBIENT_SPEED_PCT
#else
#define SC_LIGHT_AMBIENT_SPEED_PCT 100
#endif

#ifdef CONFIG_SC_LIGHT_AUDIO_SWAY_ENABLE
#define SC_LIGHT_AUDIO_SWAY_ENABLE 1
#else
#define SC_LIGHT_AUDIO_SWAY_ENABLE 0
#endif

#ifdef CONFIG_SC_LIGHT_AUDIO_SWAY_PCT
#define SC_LIGHT_AUDIO_SWAY_PCT CONFIG_SC_LIGHT_AUDIO_SWAY_PCT
#else
#define SC_LIGHT_AUDIO_SWAY_PCT 0
#endif

#ifdef CONFIG_SC_LIGHT_AUDIO_SWAY_MS
#define SC_LIGHT_AUDIO_SWAY_MS CONFIG_SC_LIGHT_AUDIO_SWAY_MS
#else
#define SC_LIGHT_AUDIO_SWAY_MS 1400
#endif

static bool s_light_enabled = true;
static int s_brightness_target_pct = SC_LIGHT_DEFAULT_BRIGHTNESS_PCT;
static float s_brightness_current_pct = 0.0f;
static uint16_t s_render_frame_index = 0;
static float s_light_time_s = 0.0f;
static uint8_t *s_rgb_buf;
static bool s_startup_anim_active = true;
static float s_startup_anim_elapsed_s = 0.0f;
static float s_audio_sway_elapsed_s = 0.0f;
static float s_audio_sway_amp_pct = 0.0f;
static bool s_audio_sway_active = false;
static float s_fluct_phase_a = 0.0f;
static float s_fluct_phase_b = 2.1f;
static float s_fluct_phase_c = 1.3f;
static float s_fluct_phase_d = 3.8f;
static float s_fluct_speed_a = 0.21f;
static float s_fluct_speed_b = 0.11f;
static float s_fluct_speed_c = 0.16f;
static float s_fluct_speed_d = 0.07f;

#define SC_LIGHT_BRIGHTNESS_STEP_PCT (5)
#define SC_LIGHT_BRIGHTNESS_MIN_PCT  (5)
#define SC_LIGHT_PI                 (3.14159265359f)
#define SC_LIGHT_TRACE_DECIMATION   (5U)
#define SC_LIGHT_SETTINGS_KEY_BRIGHTNESS "light_pct"

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

static float sc_random_symm(void)
{
    return (((float)(esp_random() & 0xFFFFU) / 65535.0f) * 2.0f) - 1.0f;
}

static float sc_startup_envelope(float dt_s)
{
    if (!s_startup_anim_active) {
        return 1.0f;
    }

    s_startup_anim_elapsed_s += dt_s;
    const float dur_s = (float)SC_LIGHT_STARTUP_RAMP_MS / 1000.0f;
    float u = s_startup_anim_elapsed_s / dur_s;
    if (u >= 1.0f) {
        s_startup_anim_active = false;
        return 1.0f;
    }

    const float ease = u * u * (3.0f - 2.0f * u);
    const float over_amp = (float)SC_LIGHT_STARTUP_OVERSHOOT_PCT / 100.0f;
    const float overshoot = over_amp * expf(-6.0f * (1.0f - u)) * sinf(12.0f * u);
    return sc_clampf(ease + overshoot, 0.0f, 1.2f);
}

static float sc_audio_sway_envelope(float dt_s)
{
    if (!s_audio_sway_active) {
        return 0.0f;
    }
    s_audio_sway_elapsed_s += dt_s;
    const float dur_s = (float)SC_LIGHT_AUDIO_SWAY_MS / 1000.0f;
    float u = s_audio_sway_elapsed_s / dur_s;
    if (u >= 1.0f) {
        s_audio_sway_active = false;
        return 0.0f;
    }

    const float shape = sinf(SC_LIGHT_PI * u) * expf(-2.2f * u);
    return s_audio_sway_amp_pct * shape;
}

static void sc_fluctuation_step(float dt_s, float *brightness_pct, float *warmth_pct)
{
    *brightness_pct = 0.0f;
    *warmth_pct = 0.0f;
#if SC_LIGHT_FLUCT_ENABLE
    const float speed_scale = (float)SC_LIGHT_AMBIENT_SPEED_PCT / 100.0f;
    const float drift_scale = dt_s * speed_scale;
    s_fluct_speed_a = sc_clampf(s_fluct_speed_a + 0.10f * drift_scale * sc_random_symm(), 0.12f, 0.28f);
    s_fluct_speed_b = sc_clampf(s_fluct_speed_b + 0.07f * drift_scale * sc_random_symm(), 0.05f, 0.16f);
    s_fluct_speed_c = sc_clampf(s_fluct_speed_c + 0.09f * drift_scale * sc_random_symm(), 0.08f, 0.22f);
    s_fluct_speed_d = sc_clampf(s_fluct_speed_d + 0.06f * drift_scale * sc_random_symm(), 0.04f, 0.12f);

    s_fluct_phase_a += s_fluct_speed_a * drift_scale;
    s_fluct_phase_b += s_fluct_speed_b * drift_scale;
    s_fluct_phase_c += s_fluct_speed_c * drift_scale;
    s_fluct_phase_d += s_fluct_speed_d * drift_scale;

    *brightness_pct = (float)SC_LIGHT_FLUCT_BRIGHTNESS_PCT *
                      (0.62f * sinf(s_fluct_phase_a) + 0.38f * sinf(s_fluct_phase_b));
    *warmth_pct = (float)SC_LIGHT_FLUCT_WARMTH_PCT *
                  (0.70f * sinf(s_fluct_phase_c) + 0.30f * sinf(s_fluct_phase_d));
#endif
}

static void sc_light_service_task(void *arg)
{
    (void)arg;
    uint32_t trace_ctr = 0;
    const uint32_t period_ms = (1000U + (uint32_t)SC_LIGHT_UPDATE_HZ - 1U) / (uint32_t)SC_LIGHT_UPDATE_HZ;
    TickType_t tick = pdMS_TO_TICKS(period_ms > 0U ? period_ms : 1U);
    if (tick == 0) {
        tick = 1;
    }
    const float dt_s = ((float)tick * (float)portTICK_PERIOD_MS) / 1000.0f;
    TickType_t last_wake = xTaskGetTickCount();
    const float effective_hz = 1.0f / dt_s;

    ESP_LOGI(TAG, "light loop timing: target_hz=%d effective_hz=%.1f period_ms=%u tick=%u dt=%.4fs",
             SC_LIGHT_UPDATE_HZ, (double)effective_hz, (unsigned)period_ms, (unsigned)tick, (double)dt_s);

    while (1) {
        s_light_time_s += dt_s;
        const bool trace_this_cycle = ((trace_ctr++ % SC_LIGHT_TRACE_DECIMATION) == 0U);
        if (trace_this_cycle) {
            SC_TRACE_MARK("light", "work_start", (int32_t)s_brightness_current_pct);
        }
        int target = s_light_enabled ? s_brightness_target_pct : 0;
        if (target < 0) {
            target = 0;
        }

        if (s_brightness_current_pct < (float)target) {
            s_brightness_current_pct += (float)SC_LIGHT_RAMP_STEP_PCT;
            if (s_brightness_current_pct > target) {
                s_brightness_current_pct = (float)target;
            }
        } else if (s_brightness_current_pct > (float)target) {
            s_brightness_current_pct -= (float)SC_LIGHT_RAMP_STEP_PCT;
            if (s_brightness_current_pct < target) {
                s_brightness_current_pct = (float)target;
            }
        }

        if (s_brightness_current_pct <= 0.0f) {
            (void)sc_led_strip_if_clear();
        } else {
            const float startup_scale = sc_startup_envelope(dt_s);
            const float audio_sway_pct = sc_audio_sway_envelope(dt_s);
            float fluct_brightness_pct = 0.0f;
            float fluct_warmth_pct = 0.0f;
            sc_fluctuation_step(dt_s, &fluct_brightness_pct, &fluct_warmth_pct);

            float effective_brightness = (s_brightness_current_pct * startup_scale) + fluct_brightness_pct + audio_sway_pct;
            effective_brightness = sc_clampf(effective_brightness, 0.0f, 100.0f);
            int warmth_shift = (int)sc_clampf(fluct_warmth_pct, -100.0f, 100.0f);

            sc_light_engine_render_warm(effective_brightness, (int8_t)warmth_shift,
                                        s_light_time_s, s_render_frame_index++,
                                        s_rgb_buf, (size_t)CONFIG_SC_LED_COUNT);
            (void)sc_led_strip_if_write_rgb(s_rgb_buf, (size_t)CONFIG_SC_LED_COUNT);
        }
        if (trace_this_cycle) {
            SC_TRACE_MARK("light", "work_end", (int32_t)s_brightness_current_pct);
        }

        vTaskDelayUntil(&last_wake, tick);
    }
}

esp_err_t sc_light_service_start(void)
{
#if CONFIG_SC_LIGHT_BACKEND_LED_STRIP
    uint8_t persisted_brightness = SC_LIGHT_DEFAULT_BRIGHTNESS_PCT;
    esp_err_t load_err = sc_settings_store_load_u8(SC_LIGHT_SETTINGS_KEY_BRIGHTNESS,
                                                   (uint8_t)SC_LIGHT_DEFAULT_BRIGHTNESS_PCT,
                                                   &persisted_brightness);
    if (load_err != ESP_OK) {
        ESP_LOGW(TAG, "brightness load failed, using default: %s", esp_err_to_name(load_err));
        persisted_brightness = (uint8_t)SC_LIGHT_DEFAULT_BRIGHTNESS_PCT;
    }

    s_rgb_buf = (uint8_t *)malloc((size_t)CONFIG_SC_LED_COUNT * 3U);
    if (s_rgb_buf == NULL) {
        ESP_LOGE(TAG, "failed to allocate LED buffer");
        return ESP_ERR_NO_MEM;
    }

#if CONFIG_SC_LED_PIXEL_ORDER_RGB
    const sc_led_pixel_order_t pixel_order = SC_LED_PIXEL_ORDER_RGB;
#else
    const sc_led_pixel_order_t pixel_order = SC_LED_PIXEL_ORDER_GRB;
#endif

    ESP_RETURN_ON_ERROR(sc_led_strip_if_init(CONFIG_SC_LED_STRIP_GPIO, (size_t)CONFIG_SC_LED_COUNT, pixel_order),
                        TAG, "led strip init failed");

    s_light_enabled = true;
    s_brightness_target_pct = (int)sc_clampf((float)persisted_brightness,
                                             (float)SC_LIGHT_BRIGHTNESS_MIN_PCT,
                                             100.0f);
    s_brightness_current_pct = 0.0f;
    s_render_frame_index = 0;
    s_light_time_s = 0.0f;
    s_startup_anim_active = true;
    s_startup_anim_elapsed_s = 0.0f;
    s_audio_sway_active = false;
    s_audio_sway_elapsed_s = 0.0f;
    s_audio_sway_amp_pct = 0.0f;

    BaseType_t ok = xTaskCreate(sc_light_service_task, "sc_light", 3072, NULL, 4, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "light service started: led_count=%d gpio=%d enabled=%d target_brightness=%d%%",
             CONFIG_SC_LED_COUNT, CONFIG_SC_LED_STRIP_GPIO, s_light_enabled, s_brightness_target_pct);
#else
    ESP_LOGI(TAG, "light service enabled, but no backend selected");
#endif
    return ESP_OK;
}

esp_err_t sc_light_service_set_enabled(bool enable)
{
    s_light_enabled = enable;
    if (enable) {
        s_startup_anim_active = true;
        s_startup_anim_elapsed_s = 0.0f;
    }
    ESP_LOGI(TAG, "light %s", enable ? "enabled" : "disabled");
    return ESP_OK;
}

esp_err_t sc_light_service_toggle(void)
{
    return sc_light_service_set_enabled(!s_light_enabled);
}

esp_err_t sc_light_service_change_brightness(int delta_steps)
{
    int next = s_brightness_target_pct + (delta_steps * SC_LIGHT_BRIGHTNESS_STEP_PCT);
    if (next < SC_LIGHT_BRIGHTNESS_MIN_PCT) {
        next = SC_LIGHT_BRIGHTNESS_MIN_PCT;
    }
    if (next > 100) {
        next = 100;
    }
    s_brightness_target_pct = next;
    s_light_enabled = true;
    esp_err_t save_err = sc_settings_store_save_u8(SC_LIGHT_SETTINGS_KEY_BRIGHTNESS, (uint8_t)next);
    if (save_err != ESP_OK) {
        ESP_LOGW(TAG, "brightness save failed: %s", esp_err_to_name(save_err));
    }
    ESP_LOGI(TAG, "brightness target=%d%% light=%d", s_brightness_target_pct, s_light_enabled);
    return ESP_OK;
}

esp_err_t sc_light_service_audio_sway(bool audio_enabled)
{
#if SC_LIGHT_AUDIO_SWAY_ENABLE
    s_audio_sway_active = true;
    s_audio_sway_elapsed_s = 0.0f;
    s_audio_sway_amp_pct = audio_enabled ? (float)SC_LIGHT_AUDIO_SWAY_PCT : -(float)SC_LIGHT_AUDIO_SWAY_PCT;
#else
    (void)audio_enabled;
#endif
    return ESP_OK;
}

bool sc_light_service_get_enabled(void)
{
    return s_light_enabled;
}

uint8_t sc_light_service_get_current_brightness_percent(void)
{
    return (uint8_t)sc_clampf(s_brightness_current_pct, 0.0f, 100.0f);
}

uint8_t sc_light_service_get_target_brightness_percent(void)
{
    return (uint8_t)sc_clampf((float)s_brightness_target_pct, 0.0f, 100.0f);
}

#else

esp_err_t sc_light_service_start(void)
{
    ESP_LOGI(TAG, "light service disabled by config");
    return ESP_OK;
}

esp_err_t sc_light_service_set_enabled(bool enable)
{
    (void)enable;
    ESP_LOGW(TAG, "set enabled ignored: light service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_toggle(void)
{
    ESP_LOGW(TAG, "toggle ignored: light service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_change_brightness(int delta_steps)
{
    (void)delta_steps;
    ESP_LOGW(TAG, "change brightness ignored: light service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_audio_sway(bool audio_enabled)
{
    (void)audio_enabled;
    return ESP_ERR_NOT_SUPPORTED;
}

bool sc_light_service_get_enabled(void)
{
    return false;
}

uint8_t sc_light_service_get_current_brightness_percent(void)
{
    return 0;
}

uint8_t sc_light_service_get_target_brightness_percent(void)
{
    return 0;
}

#endif
