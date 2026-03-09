#pragma once

#include "esp_err.h"
#include "app_events.h"

esp_err_t sc_app_core_start(void);
esp_err_t sc_app_core_post_event(const sc_app_event_t *event);
