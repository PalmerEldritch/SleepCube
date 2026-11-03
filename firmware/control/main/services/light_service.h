#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#define SC_LIGHT_DEBUG_PWM_MIN_KHZ       (1U)
#define SC_LIGHT_DEBUG_PWM_MAX_KHZ       (25U)
#define SC_LIGHT_DEBUG_DUTY_BPS_MAX      (10000U)

typedef struct {
    bool manual_duty_enabled;
    uint16_t manual_duty_bps;
    uint32_t requested_frequency_hz;
    uint32_t actual_frequency_hz;
    uint8_t resolution_bits;
} sc_light_debug_status_t;

esp_err_t sc_light_service_start(void);
esp_err_t sc_light_service_set_enabled(bool enable);
esp_err_t sc_light_service_toggle(void);
esp_err_t sc_light_service_change_brightness(int delta_steps);
/** Set the persisted logical brightness; 0 is off and 100 is maximum. */
esp_err_t sc_light_service_set_brightness_percent(uint8_t percent);
esp_err_t sc_light_service_audio_sway(bool audio_enabled);
esp_err_t sc_light_service_set_pulse_attack_ms(uint16_t attack_ms);
esp_err_t sc_light_service_set_pulse_release_ms(uint16_t release_ms);
esp_err_t sc_light_service_set_pulse_peak_up_pct(uint8_t peak_up_pct);
esp_err_t sc_light_service_set_pulse_peak_down_pct(uint8_t peak_down_pct);
uint16_t sc_light_service_get_pulse_attack_ms(void);
uint16_t sc_light_service_get_pulse_release_ms(void);
uint8_t sc_light_service_get_pulse_peak_up_pct(void);
uint8_t sc_light_service_get_pulse_peak_down_pct(void);
bool sc_light_service_get_enabled(void);
uint8_t sc_light_service_get_current_brightness_percent(void);
uint8_t sc_light_service_get_target_brightness_percent(void);

/** Development-only touch mode lifecycle. */
esp_err_t sc_light_service_debug_enter_touch_mode(void);
esp_err_t sc_light_service_debug_exit_touch_mode(void);
bool sc_light_service_debug_effects_enabled(void);

/** Development-only: queue a 1..25 kHz carrier in 1 kHz steps. */
esp_err_t sc_light_service_debug_set_pwm_frequency_khz(uint8_t frequency_khz);
/** Development-only: override GUI/effects with an exact duty in basis points. */
esp_err_t sc_light_service_debug_set_pwm_duty_bps(uint16_t duty_bps);
/** Development-only: release exact-duty override and resume GUI/effects output. */
esp_err_t sc_light_service_debug_clear_pwm_duty_override(void);
/** Read the current development PWM controls and applied hardware state. */
esp_err_t sc_light_service_debug_get_status(sc_light_debug_status_t *status);
