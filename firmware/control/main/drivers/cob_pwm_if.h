#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_cob_pwm_if_init(int gpio_num, uint32_t frequency_hz);
esp_err_t sc_cob_pwm_if_set_duty_bps(uint16_t duty_bps);
esp_err_t sc_cob_pwm_if_set_frequency_hz(uint32_t frequency_hz);
esp_err_t sc_cob_pwm_if_force_off(void);
uint32_t sc_cob_pwm_if_get_frequency_hz(void);
uint8_t sc_cob_pwm_if_get_resolution_bits(void);
uint16_t sc_cob_pwm_if_get_duty_bps(void);
