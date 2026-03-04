#pragma once

#include <stdint.h>
#include "esp_err.h"

esp_err_t sc_audio_loopback_start(uint32_t sample_rate_hz);
