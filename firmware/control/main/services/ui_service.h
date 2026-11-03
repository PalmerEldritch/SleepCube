#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

esp_err_t sc_ui_service_start(void);
void sc_ui_service_audio_state_changed(bool playback_active);
void sc_ui_service_audio_volume_changed(uint8_t volume_percent);
void sc_ui_service_debug_enter(void);
void sc_ui_service_debug_exit(void);
void sc_ui_service_debug_refresh(void);
