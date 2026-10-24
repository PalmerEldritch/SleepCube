#include "light_service.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "settings_policy.h"
#include "settings_store.h"

#if CONFIG_SC_ENABLE_LIGHT
#include "esp_random.h"
#include "cob_pwm_if.h"
#include "light_pwm_policy.h"
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

static bool s_light_enabled = true;
static int s_brightness_target_pct = SC_LIGHT_DEFAULT_BRIGHTNESS_PCT;
static float s_brightness_current_pct = 0.0f;
static bool s_startup_anim_active = true;
static float s_startup_anim_elapsed_s = 0.0f;
static float s_audio_sway_elapsed_s = 0.0f;
static float s_audio_sway_peak_pct = 0.0f;
static bool s_audio_sway_active = false;
static uint16_t s_pulse_attack_ms = 260U;
static uint16_t s_pulse_release_ms = 900U;
static uint8_t s_pulse_peak_up_pct = 19U;
static uint8_t s_pulse_peak_down_pct = 16U;
static float s_fluct_phase_a = 0.0f;
static float s_fluct_phase_b = 2.1f;
static float s_fluct_speed_a = 0.21f;
static float s_fluct_speed_b = 0.11f;
static portMUX_TYPE s_debug_pwm_lock = portMUX_INITIALIZER_UNLOCKED;
static bool s_debug_manual_duty_enabled;
static uint16_t s_debug_manual_duty_bps;
static uint32_t s_debug_requested_frequency_hz = CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ;
static uint32_t s_debug_applied_request_hz = CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ;
static uint32_t s_debug_actual_frequency_hz;
static uint8_t s_debug_resolution_bits;

#define SC_LIGHT_BRIGHTNESS_STEP_PCT (5)
#define SC_LIGHT_BRIGHTNESS_MIN_PCT  (0)
#define SC_LIGHT_TRACE_DECIMATION   (5U)
#define SC_LIGHT_SETTINGS_KEY_BRIGHTNESS "light_pct"
#define SC_LIGHT_BRIGHTNESS_RESPONSE_S   (0.22f)
#define SC_LIGHT_STARTUP_BLOOM_SCALE     (0.35f)
#define SC_LIGHT_AMBIENT_BRIGHTNESS_SCALE (1.45f)
#define SC_LIGHT_AMBIENT_SPEED_SCALE     (1.15f)
#define SC_LIGHT_PULSE_ATTACK_MIN_MS     (20U)
#define SC_LIGHT_PULSE_ATTACK_MAX_MS     (5000U)
#define SC_LIGHT_PULSE_RELEASE_MIN_MS    (20U)
#define SC_LIGHT_PULSE_RELEASE_MAX_MS    (8000U)

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

static uint16_t sc_clamp_u16(uint16_t v, uint16_t lo, uint16_t hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static uint8_t sc_clamp_u8(uint8_t v, uint8_t lo, uint8_t hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static float sc_smoothstep01(float t)
{
    t = sc_clampf(t, 0.0f, 1.0f);
    return t * t * (3.0f - (2.0f * t));
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
    const float over_amp = ((float)SC_LIGHT_STARTUP_OVERSHOOT_PCT / 100.0f) * SC_LIGHT_STARTUP_BLOOM_SCALE;
    const float overshoot = over_amp * expf(-5.5f * (1.0f - u)) * sinf(10.0f * u);
    return sc_clampf(ease + overshoot, 0.0f, 1.2f);
}

static float sc_audio_sway_envelope(float dt_s)
{
    if (!s_audio_sway_active) {
        return 0.0f;
    }

    s_audio_sway_elapsed_s += dt_s;
    const float attack_s = (float)s_pulse_attack_ms / 1000.0f;
    const float release_s = (float)s_pulse_release_ms / 1000.0f;
    const float total_s = attack_s + release_s;
    if (total_s <= 0.0f) {
        s_audio_sway_active = false;
        return 0.0f;
    }

    if (s_audio_sway_elapsed_s >= total_s) {
        s_audio_sway_active = false;
        return 0.0f;
    }

    float env = 0.0f;
    if ((attack_s > 0.0f) && (s_audio_sway_elapsed_s < attack_s)) {
        env = sc_smoothstep01(s_audio_sway_elapsed_s / attack_s);
    } else if (release_s > 0.0f) {
        const float release_elapsed_s = s_audio_sway_elapsed_s - attack_s;
        env = 1.0f - sc_smoothstep01(release_elapsed_s / release_s);
    }

    return s_audio_sway_peak_pct * env;
}

static float sc_fluctuation_step(float dt_s)
{
#if SC_LIGHT_FLUCT_ENABLE
    const float speed_scale = ((float)SC_LIGHT_AMBIENT_SPEED_PCT / 100.0f) * SC_LIGHT_AMBIENT_SPEED_SCALE;
    const float drift_scale = dt_s * speed_scale;
    s_fluct_speed_a = sc_clampf(s_fluct_speed_a + 0.08f * drift_scale * sc_random_symm(), 0.08f, 0.18f);
    s_fluct_speed_b = sc_clampf(s_fluct_speed_b + 0.05f * drift_scale * sc_random_symm(), 0.03f, 0.11f);

    s_fluct_phase_a += s_fluct_speed_a * drift_scale;
    s_fluct_phase_b += s_fluct_speed_b * drift_scale;

    return ((float)SC_LIGHT_FLUCT_BRIGHTNESS_PCT * SC_LIGHT_AMBIENT_BRIGHTNESS_SCALE) *
           (0.62f * sinf(s_fluct_phase_a) + 0.38f * sinf(s_fluct_phase_b));
#else
    (void)dt_s;
    return 0.0f;
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
        const bool trace_this_cycle = ((trace_ctr++ % SC_LIGHT_TRACE_DECIMATION) == 0U);
        if (trace_this_cycle) {
            SC_TRACE_MARK("light", "work_start", (int32_t)s_brightness_current_pct);
        }
        uint32_t requested_frequency_hz;
        bool manual_duty_enabled;
        uint16_t manual_duty_bps;
        portENTER_CRITICAL(&s_debug_pwm_lock);
        requested_frequency_hz = s_debug_requested_frequency_hz;
        manual_duty_enabled = s_debug_manual_duty_enabled;
        manual_duty_bps = s_debug_manual_duty_bps;
        portEXIT_CRITICAL(&s_debug_pwm_lock);

        if (requested_frequency_hz != s_debug_applied_request_hz) {
            const esp_err_t frequency_err =
                sc_cob_pwm_if_set_frequency_hz(requested_frequency_hz);
            const uint32_t actual_frequency_hz =
                (frequency_err == ESP_OK) ? sc_cob_pwm_if_get_frequency_hz() : 0U;
            const uint8_t resolution_bits =
                (frequency_err == ESP_OK) ? sc_cob_pwm_if_get_resolution_bits() : 0U;
            portENTER_CRITICAL(&s_debug_pwm_lock);
            if (frequency_err == ESP_OK) {
                s_debug_applied_request_hz = requested_frequency_hz;
                s_debug_actual_frequency_hz = actual_frequency_hz;
                s_debug_resolution_bits = resolution_bits;
            } else if (s_debug_requested_frequency_hz == requested_frequency_hz) {
                /* Do not discard a newer console request queued while the
                 * failed hardware reconfiguration was in progress. */
                s_debug_requested_frequency_hz = s_debug_applied_request_hz;
            }
            portEXIT_CRITICAL(&s_debug_pwm_lock);
            if (frequency_err != ESP_OK) {
                ESP_LOGE(TAG, "debug PWM frequency change failed: %s",
                         esp_err_to_name(frequency_err));
            }
        }

        int target = s_light_enabled ? s_brightness_target_pct : 0;
        if (target < 0) {
            target = 0;
        }

        if (fabsf(s_brightness_current_pct - (float)target) < 0.05f) {
            s_brightness_current_pct = (float)target;
        } else {
            const float alpha = 1.0f - expf(-dt_s / SC_LIGHT_BRIGHTNESS_RESPONSE_S);
            s_brightness_current_pct += ((float)target - s_brightness_current_pct) * alpha;
        }

        uint16_t output_duty_bps = 0U;
        if (s_brightness_current_pct <= 0.0f) {
            output_duty_bps = 0U;
        } else {
            const float startup_scale = sc_startup_envelope(dt_s);
            const bool effects_enabled = s_light_enabled && (s_brightness_target_pct > 0);
            const float audio_sway_pct = effects_enabled ? sc_audio_sway_envelope(dt_s) : 0.0f;
            const float fluct_brightness_pct = effects_enabled ? sc_fluctuation_step(dt_s) : 0.0f;

            float effective_brightness =
                (s_brightness_current_pct * startup_scale) + fluct_brightness_pct + audio_sway_pct;
            if (effects_enabled && !s_startup_anim_active) {
                effective_brightness = fmaxf(effective_brightness, 1.0f);
            }
            effective_brightness = sc_clampf(effective_brightness, 0.0f, 100.0f);
            output_duty_bps = sc_light_pwm_map_brightness(
                effective_brightness,
                (uint8_t)CONFIG_SC_LIGHT_PWM_MIN_DUTY_PCT,
                (uint8_t)CONFIG_SC_LIGHT_PWM_MAX_DUTY_PCT);
        }
        if (manual_duty_enabled) {
            output_duty_bps = manual_duty_bps;
        }
        ESP_ERROR_CHECK_WITHOUT_ABORT(sc_cob_pwm_if_set_duty_bps(output_duty_bps));
        if (trace_this_cycle) {
            SC_TRACE_MARK("light", "work_end", (int32_t)s_brightness_current_pct);
        }

        vTaskDelayUntil(&last_wake, tick);
    }
}

esp_err_t sc_light_service_start(void)
{
    uint8_t persisted_brightness = SC_LIGHT_DEFAULT_BRIGHTNESS_PCT;
    esp_err_t load_err = sc_settings_store_load_u8(SC_LIGHT_SETTINGS_KEY_BRIGHTNESS,
                                                   (uint8_t)SC_LIGHT_DEFAULT_BRIGHTNESS_PCT,
                                                   &persisted_brightness);
    if (load_err != ESP_OK) {
        ESP_LOGW(TAG, "brightness load failed, using default: %s", esp_err_to_name(load_err));
        persisted_brightness = (uint8_t)SC_LIGHT_DEFAULT_BRIGHTNESS_PCT;
    }
    const uint8_t validated_brightness = sc_settings_bound_u8(
        persisted_brightness, SC_LIGHT_BRIGHTNESS_MIN_PCT, 100U);
    if (validated_brightness != persisted_brightness) {
        ESP_LOGW(TAG, "saved brightness=%u%% is invalid; clamped to %u%%",
                 (unsigned)persisted_brightness,
                 (unsigned)validated_brightness);
        const esp_err_t repair_err = sc_settings_store_save_u8(
            SC_LIGHT_SETTINGS_KEY_BRIGHTNESS, validated_brightness);
        if (repair_err != ESP_OK) {
            ESP_LOGW(TAG, "brightness repair save failed: %s",
                     esp_err_to_name(repair_err));
        }
        persisted_brightness = validated_brightness;
    }

    ESP_RETURN_ON_ERROR(sc_cob_pwm_if_init(CONFIG_SC_LIGHT_PWM_GPIO,
                                           CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ),
                        TAG, "COB PWM init failed");
    if (!sc_light_pwm_limits_valid((uint8_t)CONFIG_SC_LIGHT_PWM_MIN_DUTY_PCT,
                                   (uint8_t)CONFIG_SC_LIGHT_PWM_MAX_DUTY_PCT)) {
        (void)sc_cob_pwm_if_force_off();
        ESP_LOGE(TAG, "invalid PWM duty floor/cap; output forced off");
        return ESP_ERR_INVALID_ARG;
    }

    s_light_enabled = true;
    s_brightness_target_pct = (int)sc_clampf((float)persisted_brightness,
                                             (float)SC_LIGHT_BRIGHTNESS_MIN_PCT,
                                             100.0f);
    s_brightness_current_pct = 0.0f;
    s_startup_anim_active = true;
    s_startup_anim_elapsed_s = 0.0f;
    s_audio_sway_active = false;
    s_audio_sway_elapsed_s = 0.0f;
    s_audio_sway_peak_pct = 0.0f;
    const uint32_t actual_frequency_hz = sc_cob_pwm_if_get_frequency_hz();
    const uint8_t resolution_bits = sc_cob_pwm_if_get_resolution_bits();
    portENTER_CRITICAL(&s_debug_pwm_lock);
    s_debug_manual_duty_enabled = false;
    s_debug_manual_duty_bps = 0U;
    s_debug_requested_frequency_hz = CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ;
    s_debug_applied_request_hz = CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ;
    s_debug_actual_frequency_hz = actual_frequency_hz;
    s_debug_resolution_bits = resolution_bits;
    portEXIT_CRITICAL(&s_debug_pwm_lock);

    BaseType_t ok = xTaskCreate(sc_light_service_task, "sc_light", 3072, NULL, 4, NULL);
    if (ok != pdPASS) {
        return ESP_FAIL;
    }
    ESP_LOGI(TAG,
             "light service started: backend=cob_pwm gpio=%d frequency=%dHz duty=%d..%d%% enabled=%d target_brightness=%d%%",
             CONFIG_SC_LIGHT_PWM_GPIO,
             CONFIG_SC_LIGHT_PWM_FREQUENCY_HZ,
             CONFIG_SC_LIGHT_PWM_MIN_DUTY_PCT,
             CONFIG_SC_LIGHT_PWM_MAX_DUTY_PCT,
             s_light_enabled,
             s_brightness_target_pct);
    return ESP_OK;
}

esp_err_t sc_light_service_set_enabled(bool enable)
{
    s_light_enabled = enable;
    if (enable) {
        s_startup_anim_active = true;
        s_startup_anim_elapsed_s = 0.0f;
    } else {
        s_audio_sway_active = false;
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
    return sc_light_service_set_brightness_percent((uint8_t)next);
}

esp_err_t sc_light_service_set_brightness_percent(uint8_t percent)
{
    int next = (int)percent;
    if (next > 100) {
        next = 100;
    }

    if (s_brightness_target_pct == next) {
        return ESP_OK;
    }

    s_brightness_target_pct = next;
    s_light_enabled = true;
    if (next == 0) {
        s_audio_sway_active = false;
    }
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
    if (!s_light_enabled || (s_brightness_target_pct <= 0)) {
        s_audio_sway_active = false;
        s_audio_sway_elapsed_s = 0.0f;
        s_audio_sway_peak_pct = 0.0f;
        return ESP_OK;
    }
    s_audio_sway_active = true;
    s_audio_sway_elapsed_s = 0.0f;
    s_audio_sway_peak_pct = audio_enabled
                                ? (float)s_pulse_peak_up_pct
                                : -(float)s_pulse_peak_down_pct;
#else
    (void)audio_enabled;
#endif
    return ESP_OK;
}

esp_err_t sc_light_service_set_pulse_attack_ms(uint16_t attack_ms)
{
    s_pulse_attack_ms = sc_clamp_u16(attack_ms, SC_LIGHT_PULSE_ATTACK_MIN_MS, SC_LIGHT_PULSE_ATTACK_MAX_MS);
    return ESP_OK;
}

esp_err_t sc_light_service_set_pulse_release_ms(uint16_t release_ms)
{
    s_pulse_release_ms = sc_clamp_u16(release_ms, SC_LIGHT_PULSE_RELEASE_MIN_MS, SC_LIGHT_PULSE_RELEASE_MAX_MS);
    return ESP_OK;
}

esp_err_t sc_light_service_set_pulse_peak_up_pct(uint8_t peak_up_pct)
{
    s_pulse_peak_up_pct = sc_clamp_u8(peak_up_pct, 0U, 100U);
    return ESP_OK;
}

esp_err_t sc_light_service_set_pulse_peak_down_pct(uint8_t peak_down_pct)
{
    s_pulse_peak_down_pct = sc_clamp_u8(peak_down_pct, 0U, 100U);
    return ESP_OK;
}

uint16_t sc_light_service_get_pulse_attack_ms(void)
{
    return s_pulse_attack_ms;
}

uint16_t sc_light_service_get_pulse_release_ms(void)
{
    return s_pulse_release_ms;
}

uint8_t sc_light_service_get_pulse_peak_up_pct(void)
{
    return s_pulse_peak_up_pct;
}

uint8_t sc_light_service_get_pulse_peak_down_pct(void)
{
    return s_pulse_peak_down_pct;
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

esp_err_t sc_light_service_debug_set_pwm_frequency_khz(uint8_t frequency_khz)
{
    if ((frequency_khz < SC_LIGHT_DEBUG_PWM_MIN_KHZ) ||
        (frequency_khz > SC_LIGHT_DEBUG_PWM_MAX_KHZ)) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_debug_pwm_lock);
    s_debug_requested_frequency_hz = (uint32_t)frequency_khz * 1000U;
    portEXIT_CRITICAL(&s_debug_pwm_lock);
    return ESP_OK;
}

esp_err_t sc_light_service_debug_set_pwm_duty_bps(uint16_t duty_bps)
{
    if (duty_bps > SC_LIGHT_DEBUG_DUTY_BPS_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_debug_pwm_lock);
    s_debug_manual_duty_bps = duty_bps;
    s_debug_manual_duty_enabled = true;
    portEXIT_CRITICAL(&s_debug_pwm_lock);
    return ESP_OK;
}

esp_err_t sc_light_service_debug_clear_pwm_duty_override(void)
{
    portENTER_CRITICAL(&s_debug_pwm_lock);
    s_debug_manual_duty_enabled = false;
    portEXIT_CRITICAL(&s_debug_pwm_lock);
    return ESP_OK;
}

esp_err_t sc_light_service_debug_get_status(sc_light_debug_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    portENTER_CRITICAL(&s_debug_pwm_lock);
    status->manual_duty_enabled = s_debug_manual_duty_enabled;
    status->manual_duty_bps = s_debug_manual_duty_bps;
    status->requested_frequency_hz = s_debug_requested_frequency_hz;
    status->actual_frequency_hz = s_debug_actual_frequency_hz;
    status->resolution_bits = s_debug_resolution_bits;
    portEXIT_CRITICAL(&s_debug_pwm_lock);
    return ESP_OK;
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

esp_err_t sc_light_service_set_brightness_percent(uint8_t percent)
{
    (void)percent;
    ESP_LOGW(TAG, "set brightness ignored: light service disabled");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_audio_sway(bool audio_enabled)
{
    (void)audio_enabled;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_set_pulse_attack_ms(uint16_t attack_ms)
{
    (void)attack_ms;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_set_pulse_release_ms(uint16_t release_ms)
{
    (void)release_ms;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_set_pulse_peak_up_pct(uint8_t peak_up_pct)
{
    (void)peak_up_pct;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_set_pulse_peak_down_pct(uint8_t peak_down_pct)
{
    (void)peak_down_pct;
    return ESP_ERR_NOT_SUPPORTED;
}

uint16_t sc_light_service_get_pulse_attack_ms(void)
{
    return 0U;
}

uint16_t sc_light_service_get_pulse_release_ms(void)
{
    return 0U;
}

uint8_t sc_light_service_get_pulse_peak_up_pct(void)
{
    return 0U;
}

uint8_t sc_light_service_get_pulse_peak_down_pct(void)
{
    return 0U;
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

esp_err_t sc_light_service_debug_set_pwm_frequency_khz(uint8_t frequency_khz)
{
    (void)frequency_khz;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_debug_set_pwm_duty_bps(uint16_t duty_bps)
{
    (void)duty_bps;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_debug_clear_pwm_duty_override(void)
{
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t sc_light_service_debug_get_status(sc_light_debug_status_t *status)
{
    (void)status;
    return ESP_ERR_NOT_SUPPORTED;
}

#endif
