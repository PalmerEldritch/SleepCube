#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_cob_pwm_if_init(int gpio_num, uint32_t frequency_hz);
esp_err_t sc_cob_pwm_if_set_duty_bps(uint16_t duty_bps);
esp_err_t sc_cob_pwm_if_force_off(void);
