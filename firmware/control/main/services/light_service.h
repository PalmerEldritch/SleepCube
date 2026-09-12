#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_light_service_start(void);
esp_err_t sc_light_service_set_enabled(bool enable);
esp_err_t sc_light_service_toggle(void);
esp_err_t sc_light_service_change_brightness(int delta_steps);
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
