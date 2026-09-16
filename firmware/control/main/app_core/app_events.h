#pragma once

#include <stdint.h>

typedef enum {
    SC_APP_EVT_BOOT = 0,
    SC_APP_EVT_UI_AUDIO_TOGGLE,
    SC_APP_EVT_UI_AUDIO_STOP,
    SC_APP_EVT_UI_VOLUME_STEP,
    SC_APP_EVT_UI_LIGHT_STEP,
} sc_app_event_type_t;

typedef struct {
    sc_app_event_type_t type;
    int32_t value;
} sc_app_event_t;
